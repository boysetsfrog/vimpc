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
      //! Start vimpc
      void Run();

   private:
      //! Read input from the screen
      int  Input() const;

      //! Handle the input using the currently active mode
      bool Handle(int input);

   private:
      //! All available modes
      typedef enum
      {
         Command,
         Normal,
         Search,
         ModeCount
      } Mode;

      typedef std::map<Mode, Ui::Handler *> HandlerTable;

   private:
      //! Check if the given \p input will require the curently active mode
      //! to be changed
      bool RequiresModeChange(int input) const;

      //! Determines the mode that will be active after the next call to
      //! ChangeMode with the \p input given
      Mode ModeAfterInput(int input)     const;

      //! Change the currently active mode based on \p input
      void ChangeMode(int input);

   private:
      Mode         currentMode_;
      HandlerTable handlerTable_;

      Settings   & settings_;
      Ui::Screen   screen_;
      Mpc::Client  client_;
   };
}

#endif
