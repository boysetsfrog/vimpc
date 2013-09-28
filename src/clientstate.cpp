/*
   Vimpc
   Copyright (C) 2010 - 2013 Nathan Sweetman

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

   mpdclient.cpp - provides interaction with the music player daemon
   */

#include "clientstate.hpp"
#include "mpdclient.hpp"

#include "assert.hpp"
#include "events.hpp"
#include "screen.hpp"
#include "settings.hpp"
#include "vimpc.hpp"

using namespace Mpc;

// Mpc::Client Implementation
ClientState::ClientState(Main::Vimpc * vimpc, Main::Settings & settings, Ui::Screen & screen) :
   vimpc_                (vimpc),
   settings_             (settings),
   screen_               (screen),

   hostname_             (""),
   port_                 (0),
   timeSinceUpdate_      (0),
   timeSinceSong_        (0),

   volume_               (100),
   mute_                 (false),
   updating_             (false),
   random_               (false),
   repeat_               (false),
   single_               (false),
   consume_              (false),
   crossfade_            (false),
   crossfadeTime_        (0),
   elapsed_              (0),

   currentSong_          (NULL),
   currentSongId_        (-1),
   totalNumberOfSongs_   (0),
   currentSongURI_       (""),
   currentState_         ("Disconnected")
{
   Main::Vimpc::EventHandler(Event::Disconnected, [this] (EventData const & Data)
   { 
      this->volume_             = -1;
      this->mute_               = false;
      this->updating_           = false;
      this->random_             = false; 
      this->repeat_             = false; 
      this->single_             = false; 
      this->consume_            = false; 
      this->crossfade_          = false;
      this->crossfadeTime_      = 0;

      this->totalNumberOfSongs_ = 0;
   });

   Main::Vimpc::EventHandler(Event::ChangeHost, [this] (EventData const & Data)
   { 
      this->hostname_ = Data.hostname; 
      this->port_     = Data.port; 
   });

   Main::Vimpc::EventHandler(Event::Random, [this] (EventData const & Data)
   { this->random_ = Data.state; });

   Main::Vimpc::EventHandler(Event::Consume, [this] (EventData const & Data)
   { this->consume_ = Data.state; });

   Main::Vimpc::EventHandler(Event::Repeat, [this] (EventData const & Data)
   { this->repeat_ = Data.state; });

   Main::Vimpc::EventHandler(Event::Single, [this] (EventData const & Data)
   { this->single_ = Data.state; });

   Main::Vimpc::EventHandler(Event::Mute, [this] (EventData const & Data)
   { this->mute_ = Data.state; });

   Main::Vimpc::EventHandler(Event::Crossfade, [this] (EventData const & Data)
   { this->crossfade_ = Data.state; });

   Main::Vimpc::EventHandler(Event::CrossfadeTime, [this] (EventData const & Data)
   { this->crossfadeTime_ = Data.value; });

   Main::Vimpc::EventHandler(Event::TotalSongCount, [this] (EventData const & Data)
   { this->totalNumberOfSongs_ = Data.count; });

   Main::Vimpc::EventHandler(Event::Update, [this] (EventData const & Data)
   { this->updating_ = true; });

   Main::Vimpc::EventHandler(Event::UpdateComplete, [this] (EventData const & Data)
   { this->updating_ = false; });

   Main::Vimpc::EventHandler(Event::Volume, [this] (EventData const & Data)
   { this->volume_ = Data.value; });
}

ClientState::~ClientState()
{
}

std::string ClientState::Hostname()
{
   return hostname_;
}

uint16_t ClientState::Port()
{
   return port_;
}

bool ClientState::Connected() const
{
   return false; //(connection_ != NULL);
}

bool ClientState::Random() const
{
   return random_;
}

bool ClientState::Single() const
{
   return single_;
}

bool ClientState::Consume() const
{
   return consume_;
}

bool ClientState::Repeat() const
{
   return repeat_;
}

int32_t ClientState::Crossfade() const
{
   if (crossfade_ == true)
   {
      return crossfadeTime_;
   }

   return 0;
}

int32_t ClientState::Volume() const
{
   return volume_;
}


bool ClientState::Mute() const
{
   return mute_;
}

bool ClientState::IsUpdating() const
{
   return updating_;
}

std::string ClientState::CurrentState()
{
   return currentState_;
}


std::string ClientState::GetCurrentSongURI()
{
   return currentSongURI_;
}

int32_t ClientState::GetCurrentSongPos()
{
   return currentSongId_;
}

uint32_t ClientState::TotalNumberOfSongs()
{
   return totalNumberOfSongs_;
}

void ClientState::DisplaySongInformation()
{
   static char durationStr[128];

   if ((Connected() == true) && (CurrentState() != "Stopped"))
   {
      if (currentSong_ != NULL)
      {
         uint32_t     const duration = mpd_song_get_duration(currentSong_);
         uint32_t     const elapsed  = elapsed_;
         uint32_t     const remain   = (duration > elapsed) ? duration - elapsed : 0;
         char const * const cArtist  = mpd_song_get_tag(currentSong_, MPD_TAG_ARTIST, 0);
         char const * const cTitle   = mpd_song_get_tag(currentSong_, MPD_TAG_TITLE, 0);
         std::string  const artist   = (cArtist == NULL) ? "Unknown" : cArtist;
         std::string  const title    = (cTitle  == NULL) ? "Unknown" : cTitle;

         screen_.SetStatusLine("[%5u] %s - %s", GetCurrentSongPos() + 1, artist.c_str(), title.c_str());


         if (settings_.Get(Setting::TimeRemaining) == false)
         {
            snprintf(durationStr, 127, "[%d:%.2d/%d:%.2d]",
                     SecondsToMinutes(elapsed),  RemainingSeconds(elapsed),
                     SecondsToMinutes(duration), RemainingSeconds(duration));
         }
         else
         {
            snprintf(durationStr, 127, "[-%d:%.2d/%d:%.2d]",
                     SecondsToMinutes(remain),  RemainingSeconds(remain),
                     SecondsToMinutes(duration), RemainingSeconds(duration));
         }

         screen_.MoveSetStatus(screen_.MaxColumns() - strlen(durationStr), "%s", durationStr);
         screen_.SetProgress(static_cast<double>(elapsed) / duration);
      }
      else
      {
         screen_.SetProgress(0);
      }
   }
   else
   {
      screen_.SetStatusLine("%s","");
      screen_.SetProgress(0);
   }
}


long ClientState::TimeSinceUpdate()
{
   return timeSinceUpdate_;
}

/* vim: set sw=3 ts=3: */
