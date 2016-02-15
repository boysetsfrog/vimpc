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

#include "events.hpp"
#include "vimpc.hpp"
#include "window/debug.hpp"

using namespace Main;

static std::list<std::string>             Queue;
static Mutex                              QueueMutex;
static Atomic(bool)                       Running(true);
static ConditionVariable                  Condition;

LyricsLoader & LyricsLoader::Instance()
{
   static LyricsLoader loader_;
   return loader_;
}

LyricsLoader::LyricsLoader() :
	loaded_				 (false),
	loading_				 (false),
	artist_				 (""),
	title_				 (""),
	uri_  				 (""),
	lyrics_				 (Main::LyricsBuffer()),
   lyricsThread_      (Thread(&LyricsLoader::LyricsQueueExecutor, this, this))
{
}

LyricsLoader::~LyricsLoader()
{
	Running = false;
   Condition.notify_all();

	lyricsThread_.join();
}


void LyricsLoader::Load(Mpc::Song * song)
{
	UniqueLock<Mutex> Lock(QueueMutex);

	Queue.clear();

	Debug("Called to load lyrics");
	
	if (song) {
		if (((loaded_ == false) || (loading_ == false)) &&
			 ((song->Artist() != artist_) || (song->Title() != title_) || (song->URI() != uri_)))
		{
			loaded_  = false;
			loading_ = true;
			artist_ = song->Artist();
			title_  = song->Title();
			uri_    = song->URI();
			Queue.push_back(uri_);
			Debug("Notifying lyrics loader");
			Condition.notify_all();
		}
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
					Debug("Found lyrics");

					while (std::getline(stream, line))
					{
						lyrics_.Add(line);
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
