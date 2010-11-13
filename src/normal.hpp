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
#include "mpdclient.hpp"
#include "player.hpp"
#include "playlist.hpp"
#include "screen.hpp"

namespace Main
{
   class Vimpc;
   class Settings;
}

namespace Ui
{
   // Handles all input received whilst in normal mode
   class Normal : public Handler, public Player
   {
   public:
      Normal(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings);
      ~Normal();

   public: // Ui::Handler
      void InitialiseMode();
      void FinaliseMode();
      bool Handle(int input);
      bool CausesModeToStart(int input);

   private:
      bool Confirm(uint32_t count);
      bool RepeatLastAction(uint32_t count);

   private: //Scrolling
      typedef enum { Line, Page }          Size;
      typedef enum { Up, Down }            Direction;
      typedef enum { Current, Start, End } Location;

      template <Size SIZE, Direction DIRECTION>
      bool Scroll(uint32_t count);
      
      template <Location LOCATION>
      bool ScrollTo(uint32_t count);

   private:
      ModeWindow *  window_;
      uint32_t      actionCount_;
      int32_t       lastAction_;
      uint32_t      lastActionCount_;

      typedef bool (Ui::Normal::*ptrToMember)(uint32_t);
      typedef std::map<int, ptrToMember> ActionTable;
      ActionTable   actionTable_;

      Mpc::Client &    client_;
      Ui::Screen  &    screen_;
      Main::Settings & settings_;

   };

   // Implementation of scrolling functions
   template <Normal::Size SIZE, Normal::Direction DIRECTION>
   bool Normal::Scroll(uint32_t count)
   {
      if (SIZE == Page)
      {
        count *= ((screen_.MaxRows() + 1) / 2);
      }

      count *= (DIRECTION == Up) ? -1 : 1;
      screen_.Scroll(count);
      return true;
   }

   template <Normal::Location LOCATION>
   bool Normal::ScrollTo(uint32_t count)
   {
      switch (LOCATION)
      {
         case Current:
            screen_.PlaylistWindow().ScrollTo(GetCurrentSong() + 1);
            break;
         case Start:
            screen_.ScrollTo(0);
            break;
         case End:
            screen_.ScrollTo(client_.TotalNumberOfSongs());
            break;
         default:
            ASSERT(false);
      }
      return true;
   }

}

#endif
