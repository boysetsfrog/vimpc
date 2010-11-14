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

   search.hpp - handles searching the current window
   */

#ifndef __UI__SEARCH
#define __UI__SEARCH

#include <string>
#include <map>
#include <vector>

#include "handler.hpp"
#include "inputmode.hpp"
#include "modewindow.hpp"
#include "player.hpp"

namespace Main
{
   class Settings;
   class Vimpc;
}

namespace Ui
{
   class Screen;

   // Handles all input received whilst in search mode
   class Search : public InputMode, public Player
   {

   public:
      typedef enum { Next,     Previous  } Skip;
      typedef enum { Forwards, Backwards, DirectionCount } Direction;

   public:
      Search(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings, Direction direction = Forwards);
      ~Search();

   public:
      void InitialiseMode(int input);
      bool CausesModeToStart(int input);

      bool SearchResult(Skip skip, uint32_t count);
      bool SearchWindow(Direction direction, std::string search, uint32_t count);

   private:
      Direction GetDirectionForInput(int input) const;

      bool CheckForMatch(std::string const & search, int32_t songId, uint32_t & count);

   private:
      bool InputModeHandler(std::string input);
      char const * const Prompt();

   private: 
      Direction        direction_;
      std::string      lastSearch_;
      char             prompt_[DirectionCount];
      Main::Settings & settings_;
      Ui::Screen     & screen_;

  };
}

#endif
