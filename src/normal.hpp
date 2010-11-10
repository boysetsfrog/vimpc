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

   normal.hpp - normal mode input handling 
   */

#ifndef __UI__NORMAL
#define __UI__NORMAL

#include <map>

#include "handler.hpp"
#include "modewindow.hpp"
#include "player.hpp"

namespace Main
{
   class Vimpc;
}

namespace Ui
{
   // Handles all input received whilst in normal mode
   class Normal : public Handler, public Player
   {
   public:
      Normal(Ui::Screen & screen, Mpc::Client & client);
      ~Normal();

   public: // Ui::Handler Inherits
      void InitialiseMode();
      void FinaliseMode();
      bool Handle(int input);
      bool CausesModeStart(int input);

   private:
      typedef enum
      {
         Up,
         Down
      } Direction;

   private:
      bool Confirm(uint32_t count);
      bool ScrollToCurrent(uint32_t count);
      bool ScrollToStart(uint32_t count);
      bool ScrollToEnd(uint32_t count);
      bool ScrollPageUp(uint32_t);
      bool ScrollPageDown(uint32_t);
      bool ScrollPage(Direction, int64_t);
      bool ScrollUp(uint32_t);
      bool ScrollDown(uint32_t);
      bool Scroll(Direction, int64_t);

      bool RepeatLastAction(uint32_t);

   private:
      ModeWindow * window_;

      uint32_t actionCount_;
      int32_t  lastAction_;
      uint32_t lastActionCount_;

      Mpc::Client & client_;
      Ui::Screen  & screen_;

      typedef bool (Ui::Normal::*ptrToMember)(uint32_t);
      typedef std::map<int, ptrToMember> ActionTable;
      ActionTable actionTable_;
   };
}

#endif
