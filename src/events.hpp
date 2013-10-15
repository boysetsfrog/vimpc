/*
   Vimpc
   Copyright (C) 2010 - 2012 Nathan Sweetman

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

   events.hpp - events that can occur in other threads that
                require handling in the main thread only
   */

#ifndef __EVENTS
#define __EVENTS

#include <string>

#include "song.hpp"

#define EVENTS \
   X(Input) /* Keyboard input event */ \
   \
   X(Connected) \
   X(Disconnected) \
   X(CurrentState) \
   X(ChangeHost) \
   X(Elapsed) \
   X(StatusUpdate) \
   X(CurrentSongId) \
   X(CurrentSongURI) \
   X(QueueUpdate) \
   X(QueueChangesStart) \
   X(ClearDatabase) \
   X(DatabaseList) \
   X(DatabaseListFile) \
   X(DatabasePath) \
   X(DatabaseSong) \
   X(AllMetaDataReady) \
   X(NewPlaylist) \
   X(PlaylistAdd) \
   X(PlaylistReplace) \
   X(PlaylistQueueReplace) \
   X(Output) \
   X(OutputEnabled) \
   X(OutputDisabled) \
   X(CommandListSend) \
   X(Random) \
   X(Single) \
   X(Consume) \
   X(Repeat) \
   X(Crossfade) \
   X(CrossfadeTime) \
   X(Mute) \
   X(Volume) \
   X(TotalSongCount) \
   X(SearchResults) \
   X(PlaylistContents) \
   X(PlaylistContentsForRemove) \
   X(Update) \
   X(UpdateComplete) \
   X(IdleMode) \
   X(StopIdleMode) \
   X(Unknown)

namespace Mpc
{
   class Output;
   class Song;
};

namespace Event
{
   enum
   {
#define X(Number) Number,
      EVENTS
#undef X
      EventCount
   };
}

struct EventData
{
   EventData() : user(false) { }

   int32_t  input;
   int32_t  count;
   int32_t  value;
   int32_t  pos1;
   int32_t  pos2;
   uint32_t id;
   uint32_t port;
   bool     state;
   bool     user;
   std::string uri;
   std::string name;
   std::string hostname;
   std::string clientstate;
   Mpc::Song * song;
   Mpc::Output * output;

   std::vector<std::string> uris;
   std::vector<std::pair<int32_t, std::string> > posuri;
};

#endif
/* vim: set sw=3 ts=3: */
