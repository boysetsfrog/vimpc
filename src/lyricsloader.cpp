/*
   Vimpc
   Copyright (C) 2010 - 2016 Nathan Sweetman

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   lyricsloader.cpp - Threaded/automated lyrics loading
   */

#include "lyricsloader.hpp"

#include <list>
#include <iostream>
#include <sstream>
#include <string>

#include "vimpc.hpp"
#include "window/debug.hpp"

static std::list<std::string>             Queue;
static Mutex                              QueueMutex;
static Atomic(bool)                       Running(true);
static ConditionVariable                  Condition;

using namespace Main;

LyricsLoader & LyricsLoader::Instance()
{
   static LyricsLoader loader_;
   return loader_;
}

LyricsLoader::LyricsLoader() :
   loaded_            (false),
   loading_           (false),
   artist_            (""),
   title_             (""),
   uri_               (""),
   lyrics_            (Main::LyricsBuffer()),
   lyricsThread_      (Thread(&LyricsLoader::LyricsQueueExecutor, this, this))
{
   Vimpc::EventHandler(Event::CurrentSong, [this] (EventData const & Data) 
   { 
       SongChanged(Data); 
   });

   Vimpc::EventHandler(Event::Elapsed, [this] (EventData const & Data)
   {
      ElapsedUpdate(Data.value);
   });

}

LyricsLoader::~LyricsLoader()
{
   Running = false;
   Condition.notify_all();
   lyricsThread_.join();
}

void LyricsLoader::SongChanged(EventData const & Data)
{
   if ((Main::Settings::Instance().Get(Setting::AutoLyrics) == true))
   {
       if (Data.currentSong) 
       {
           std::string const artist = mpd_song_get_tag(Data.currentSong, MPD_TAG_ARTIST, 0);
           std::string const title  = mpd_song_get_tag(Data.currentSong, MPD_TAG_TITLE, 0);
           std::string const uri    = mpd_song_get_uri(Data.currentSong);
           uint32_t  const duration = mpd_song_get_duration(Data.currentSong);

           Load(artist, title, uri, duration);
        }
   }
}

void LyricsLoader::ElapsedUpdate(uint32_t elapsed)
{
   if ((Main::Settings::Instance().Get(Setting::AutoLyrics) == true) &&
       (Main::Settings::Instance().Get(Setting::AutoScrollLyrics) == true))
   {
       if (duration_ > 0) {
           uint32_t percent = (((elapsed*100)/duration_)/10);

           percent *= 10;

           if (percent != percent_)
           {
                EventData Data;
                Data.value = percent;
                percent_   = percent;
                Main::Vimpc::CreateEvent(Event::LyricsPercent, Data);
                Main::Vimpc::CreateEvent(Event::Repaint,       Data);
           }
       }
   }
}

void LyricsLoader::Load(Mpc::Song * song)
{
   if (song) {
      Load(song->Artist(), song->Title(), song->URI(), song->Duration());
   }
}

void LyricsLoader::Load(std::string artist, std::string title, std::string uri, uint32_t duration)
{
   Regex::RE titleStrip(Main::Settings::Instance().Get(Setting::LyricsStrip));

   titleStrip.ReplaceAll("", title);

   UniqueLock<Mutex> Lock(QueueMutex);

   Debug("Called to load lyrics");
   
   if (((loaded_ == false) || (loading_ == false)) &&
       ((artist != artist_) || (title != title_) || (uri != uri_)))
   {
      Queue.clear();

      loaded_  = false;
      loading_ = true;
      artist_  = artist;
      title_   = title;
      uri_     = uri;
      duration_= duration;
      percent_ = 0;
      Queue.push_back(uri_);
      Debug("Notifying lyrics loader");
      Condition.notify_all();
   }
   else
   {
      Debug("Didn't do an update, already loaded");
   }
}

void LyricsLoader::LyricsQueueExecutor(Main::LyricsLoader * loader)
{
   while (Running == true)
   {
      UniqueLock<Mutex> Lock(QueueMutex);

      if ((Queue.empty() == false) ||
          (ConditionWait(Condition, Lock, 250) != false))
      {
         if (Queue.empty() == false)
         {
            Queue.pop_front();
            Lock.unlock();
            loaded_  = false;
            loading_ = true;

            // \todo wait on the queue
            std::string const artist = Curl::escape(artist_);
            std::string const title  = Curl::escape(title_);
            LyricsFetcher::Result result;

            Debug("Attempting to find lyrics");

            for (LyricsFetcher **plugin = lyricsPlugins; *plugin != 0; ++plugin)
            {
               result = (*plugin)->fetch(artist, title);

               if (result.first == true)
                  break;
            }

            lyrics_.Clear();

            if (result.first == true)
            {
               std::stringstream stream(result.second);
               std::string line;
               std::string last_line = "";
               Debug("Found lyrics");

               while (std::getline(stream, line))
               {
                  if ((line == last_line) && (last_line == ""))
                  {
                      continue;
                  }

                  lyrics_.Add(line);
                  last_line = line;
               }
            }

            loaded_  = true;

            EventData Data;
            Main::Vimpc::CreateEvent(Event::LyricsLoaded, Data);
            Main::Vimpc::CreateEvent(Event::Repaint,      Data);
            continue;
         }
      }

      loading_ = false;
   }
}
