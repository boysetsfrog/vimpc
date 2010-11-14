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

   search.cpp - handles searching the current window
   */

#include "search.hpp"

#include "playlist.hpp"
#include "settings.hpp"
#include "vimpc.hpp"

#include <iostream>
#include <boost/regex.hpp>

using namespace Ui;

Search::Search(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings, Direction direction) :
   InputMode   (screen),
   Player      (screen, client, settings),
   direction_  (direction),
   lastSearch_ (""),
   prompt_     (),
   settings_   (settings),
   screen_     (screen)
{
   prompt_[Forwards]  = '/';
   prompt_[Backwards] = '?';
}

Search::~Search()
{
}


bool Search::CausesModeToStart(int input)
{
   return ((input == prompt_[Forwards]) || (input == prompt_[Backwards]));
}


void Search::SetDirection(Direction direction)
{
   direction_ = direction;
}

Search::Direction Search::GetDirectionForInput(int input) const
{
   Direction direction = Forwards;

   if (input == prompt_[Backwards])
   {
      direction = Backwards;
   }

   return direction;
}

bool Search::SearchResult(Skip skip, uint32_t count)
{
   Direction direction = Forwards;

   if (((direction_ == Backwards) && (skip == Next)) || 
       ((direction_ == Forwards)  && (skip == Previous)))
   {
      direction = Backwards;
   }

   SearchWindow(direction, lastSearch_, count);

   return true;
}

bool Search::SearchWindow(Direction direction, std::string search, uint32_t count)
{
   bool found = false;

   if (direction == Forwards)
   {
      for (int32_t i = screen_.PlaylistWindow().CurrentLine() + 1; ((i <= (int32_t) screen_.PlaylistWindow().LastLine()) && (found == false)); ++i)
      {
         found = CheckForMatch(search, i, count);
      }
   }
   else
   {
      for (int32_t i = screen_.PlaylistWindow().CurrentLine() - 1; ((i >= 0) && (found == false)); --i)
      {
         found = CheckForMatch(search, i, count);
      }
   }

   return true;
}

bool Search::CheckForMatch(std::string const & search, int32_t songId, uint32_t & count)
{
   boost::regex  expression(".*" + search + ".*");
   bool          found     (false);

   std::string songDescription(screen_.PlaylistWindow().GetSong(songId)->FullDescription());

   if (boost::regex_match(songDescription.c_str(), expression) == true)
   {
      screen_.PlaylistWindow().ScrollTo(songId + 1);

      --count;
      found = (count == 0);
   }

   return found;
}

char const * const Search::Prompt() 
{ 
   static char SearchPrompt[2] = "";

   SearchPrompt[0] = prompt_[direction_];
   SearchPrompt[1] = '\0';

   return SearchPrompt; 
}

bool Search::InputModeHandler(std::string input) 
{
   lastSearch_ = input;
   return SearchResult(Next, 1);
}
