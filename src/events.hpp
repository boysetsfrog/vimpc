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

#define EVENTS \
   X(Input) \
   X(Connected) \
   X(Disconnected) \
   X(StatusUpdate) \
   X(QueueUpdate) \
   X(AllMetaDataReady) \
   X(PlaylistAdd) \
   X(OutputEnabled) \
   X(OutputDisabled) \
   X(CommandListSend) \
   X(Random) \
   X(Unknown)

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
   int32_t  input;
   int32_t  pos1;
   int32_t  pos2;
   uint32_t id;
   bool     state;
   std::string uri;
};

#endif
/* vim: set sw=3 ts=3: */
