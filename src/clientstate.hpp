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

   clientstate.hpp - current state of the client/server
   */

#ifndef __MPC__CLIENTSTATE
#define __MPC__CLIENTSTATE

#include <mpd/client.h>

#include "compiler.hpp"
#include "output.hpp"
#include "screen.hpp"
#include "buffers.hpp"
#include "buffer/library.hpp"
#include "buffer/list.hpp"
#include "window/debug.hpp"

namespace Main
{
   class Settings;
   class Vimpc;
}

namespace Ui
{
   class Screen;
}

// \todo cache all the values that we can
namespace Mpc
{
   class ClientState
   {
   public:
      ClientState(Main::Vimpc * vimpc, Main::Settings & settings, Ui::Screen & screen);
      ~ClientState();

   private:
      ClientState(Client & client);
      ClientState & operator=(ClientState & client);

   public:
      // Mpd Connections
      std::string Hostname();
      uint16_t Port();
      bool Connected() const;
      bool IsSocketFile() const;

   public:
      // Toggle settings
      bool Random() const;
      bool Single() const;
      bool Consume() const;
      bool Repeat() const;
      int32_t Crossfade() const;
      int32_t Volume() const;
      bool Mute() const;
      bool IsUpdating() const;

   public:
      // Mpd Status
      std::string CurrentState() const ;
      std::string GetCurrentSongURI() const;

      uint32_t TotalNumberOfSongs();
      int32_t  GetCurrentSongPos();


   public:
      long TimeSinceUpdate();
      bool IsIdle();

   public:
      void DisplaySongInformation();

   private:
      Main::Vimpc *           vimpc_;
      Main::Settings &        settings_;
      Ui::Screen &            screen_;

      bool                    connected_;
      std::string             hostname_;
      uint16_t                port_;
      long                    timeSinceUpdate_;
      long                    timeSinceSong_;

      uint32_t                volume_;
      bool                    mute_;
      bool                    updating_;
      bool                    random_;
      bool                    repeat_;
      bool                    single_;
      bool                    consume_;
      bool                    crossfade_;
      bool                    running_;
      bool                    newSong_;
      bool                    scrollingStatus_;
      uint32_t                crossfadeTime_;
      uint32_t                elapsed_;
      uint32_t                titlePos_;
      uint32_t                waitTime_;

      mpd_song *              currentSong_;
      int32_t                 currentSongId_;
      uint32_t                totalNumberOfSongs_;
      std::string             currentSongURI_;
      std::string             currentState_;
      std::string             lastTitleStr_;
      std::thread             updateThread_; 
   };
}

#endif
/* vim: set sw=3 ts=3: */
