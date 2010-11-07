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

   vimpc.hpp - handles mode changes and input processing
   */

#ifndef __MAIN__VIMPC
#define __MAIN__VIMPC

#include <map>

#include "mpdclient.hpp"
#include "screen.hpp"

namespace Ui
{
   class Handler;
}

namespace Main
{
   class Settings;

   class Vimpc
   {
   public:
      Vimpc();
      ~Vimpc();

   public:
      void Run();

   private:
      typedef enum
      {
         Command,
         Normal,
         Search,
         ModeCount
      } Mode;

      typedef std::map<Mode, Ui::Handler *> HandlerTable;

   private:
      bool Handle(int input);

   private:
      bool RequiresModeChange() const;
      Mode ModeAfterInput() const;
      void ChangeMode();

   private:
      int Input() const;

   private:
      int          input_;
      Mode         currentMode_;

      Settings   & settings_;
      Ui::Screen   screen_;
      Mpc::Client  client_;
      HandlerTable handlerTable_;
   };
}

#endif
