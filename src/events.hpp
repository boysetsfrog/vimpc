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
   X(Input,  "Input") /* Keyboard input event */ \
   X(Connected,  "Connected") \
   X(Disconnected, "Disconnected") \
   X(CurrentState, "CurrentState") \
   X(ChangeHost, "ChangeHost") \
   X(Continue, "Continue") \
   X(Elapsed, "Elapsed") \
   X(StatusUpdate, "StatusUpdate") \
   X(Repaint, "Repaint") \
   X(CurrentSongId, "CurrentSongId") \
   X(CurrentSong, "CurrentSong") \
   X(QueueUpdate, "QueueUpdate") \
   X(QueueChangesStart, "QueueChangesStart") \
   X(ClearDatabase, "ClearDatabase") \
   X(DatabaseList, "DatabaseList") \
   X(DatabaseListFile, "DatabaseListFile") \
   X(DatabasePath, "DatabasePath") \
   X(DatabaseSong, "DatabaseSong") \
   X(AllMetaDataReady, "AllMetaDataReady") \
   X(NewPlaylist, "NewPlaylist") \
   X(PlaylistAdd, "PlaylistAdd") \
   X(PlaylistQueueReplace, "PlaylistQueueReplace") \
   X(Output, "Output") \
   X(OutputEnabled, "OutputEnabled") \
   X(OutputDisabled, "OutputDisabled") \
   X(CommandListSend, "CommandListSend") \
   X(Random, "Random") \
   X(Single, "Single") \
   X(Consume, "Consume") \
   X(Repeat, "Repeat") \
   X(Crossfade, "Crossfade") \
   X(CrossfadeTime, "CrossfadeTime") \
   X(Mute, "Mute") \
   X(Volume, "Volume") \
   X(TotalSongCount, "TotalSongCount") \
   X(SearchResults, "SearchResults") \
   X(TestResult, "TestResult") \
   X(PlaylistContents, "PlaylistContents") \
   X(PlaylistContentsForRemove, "PlaylistContentsForRemove") \
   X(Autoscroll, "Autoscroll") \
   X(Update, "Update") \
   X(UpdateComplete, "UpdateComplete") \
   X(RequirePassword, "RequirePassword") \
   X(IdleMode, "IdleMode") \
   X(StopIdleMode, "StopIdleMode") \
   X(LyricsLoaded, "LyricsLoaded") \
   X(LyricsPercent, "LyricsPercent") \
   X(DisplaySongInfo, "DisplaySongInfo") \
   X(DatabaseEnabled, "DatabaseEnabled") \
   X(Unknown, "Unknown")

namespace Mpc
{
   class Output;
   class Song;
};

namespace Event
{
   enum
   {
#define X(Number, String) Number,
      EVENTS
#undef X
      EventCount
   };
}

class EventStrings
{
public:
   static std::string Default[];
};

struct EventData
{
   EventData() :
      user  (false),
      song  (NULL),
      output(NULL),
      currentSong(NULL)
         { }

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
   mpd_song *  currentSong;

   std::vector<std::string> uris;
   std::vector<std::pair<int32_t, std::pair<Mpc::Song *, std::string> > > posuri;
};

#endif
/* vim: set sw=3 ts=3: */
