/*
   Vimpc
   Copyright (C) 2010 - 2011 Nathan Sweetman

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

#include "inputmode.hpp"
#include "player.hpp"
#include "regex.hpp"
#include "window/modewindow.hpp"

namespace Main
{
   class Settings;
}

namespace Ui
{
   class Screen;

   // Handles all input received whilst in search mode
   class Search : public InputMode
   {

   public:
      Search(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings);
      ~Search();

   public:
      typedef enum
      {
         Next,
         Previous
      } Skip;

      typedef enum
      {
         Forwards,
         Backwards,
         DirectionCount
      } Direction;

   public: //Ui::InputMode
      void Initialise(int input);
      void Finalise(int input);
      bool Handle(int input);
      bool CausesModeToStart(int input) const;

   public:
      std::string LastSearchString() const;
      Regex::Options LastSearchOptions() const;
      bool SearchResult(Skip skip, uint32_t count);

      void SetHighlightSearch(bool highlight) { highlight_ = highlight; }
      bool HighlightSearch() const { return highlight_; }

   private:
      bool SearchResult(Skip skip, std::string const & search, int32_t line, uint32_t count, bool raiseError = true);
      bool SearchWindow(Direction direction, std::string search, int32_t startLine, uint32_t count);
      bool SearchForResult(Direction direction, std::string search, uint32_t count, int32_t startLine);
      Direction SwapDirection(Direction direction) const;
      Direction GetDirectionForInput(int input) const;
      bool CheckForMatch(std::string const & search, int32_t songId, uint32_t & count);

      Regex::Options GetOptions(const std::string & search) const;
      std::string StripFlags(std::string) const;

   private: //Ui::InputMode
      bool InputStringHandler(std::string input);
      char const * Prompt() const;

   private:
      Direction           direction_;
      int32_t             currentLine_;
      std::string         lastSearch_;
      std::string         currentSearch_;
      bool                hasSearched_;
      bool                highlight_;
      char                prompt_[DirectionCount];
      Main::Settings &    settings_;
      Ui::Screen     &    screen_;

  };
}

#endif
/* vim: set sw=3 ts=3: */
