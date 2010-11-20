/*
   Vimpc
   Copyright (C) 2010 Nathan Sweetman

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

   player.hpp - common music playing related commands 
   */

#ifndef __UI__PLAYER
#define __UI__PLAYER

#include <string>
#include <stdint.h>

#include "mpdclient.hpp"
#include "screen.hpp"

namespace Main
{
   class Settings;
}

namespace Mpc
{
   class Client;
}

namespace Ui
{
   class Screen;
}

namespace Ui
{
   // Class for functionality that is shared between all
   // modes and needs to be accessed via actions or commands, etc
   class Player
   {
   public:
      Player(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings);
      virtual ~Player() = 0;

   protected:
      //Commands which may be called by the mode
      bool ClearScreen();
      bool Connect(std::string const & host, uint32_t port = 0);
      bool Echo(std::string const & arguments);
      bool Pause();
      bool Play(uint32_t id);
      bool Quit();
      bool Random(bool random);
      bool SetActiveWindow(Ui::Screen::MainWindow window);
      bool Stop();

   protected:
      typedef enum 
      { 
         Next,
         Previous
      } Skip;

      bool SkipSong(Skip skip, uint32_t count);
      //bool SkipAlbum(Skip skip, uint32_t count);
      bool SkipArtist(Skip skip, uint32_t count);

   protected:
      uint32_t GetCurrentSong() const;

   private:
      void HandleAutoScroll();

   protected:
      Ui::Screen     & screen_;
      Mpc::Client    & client_;
      Main::Settings & settings_;
   };

}

#endif
