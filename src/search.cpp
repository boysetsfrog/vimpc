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
   settings_   (settings),
   screen_     (screen)
{
}

Search::~Search()
{
}


bool Search::CausesModeToStart(int input)
{
   return ((input == '/') || (input == '?'));
}


void Search::SetDirection(Direction direction)
{
   direction_ = direction;
}

Search::Direction Search::GetDirectionForInput(int input) const
{
   Direction direction = Forwards;

   if (input == '?')
   {
      direction = Backwards;
   }

   return direction;
}

bool Search::PrevSearchResult()
{
   if (direction_ == Forwards)
   {
      SearchBackwards(lastSearch_);
   }
   else 
   {
      SearchForwards(lastSearch_);
   }

   return true;
}

bool Search::NextSearchResult()
{
   if (direction_ == Forwards)
   {
      SearchForwards(lastSearch_);
   }
   else 
   {
      SearchBackwards(lastSearch_);
   }

   return true;
}

bool Search::SearchForwards(std::string search)
{
   boost::regex  expression(".*" + search + ".*");
   boost::cmatch what;

   bool found = false;

   for (int32_t i = screen_.PlaylistWindow().CurrentLine() + 1; ((i <= (int32_t) screen_.PlaylistWindow().LastLine()) && (found == false)); ++i)
   {
      std::string songDescription(screen_.PlaylistWindow().GetSong(i)->FullDescription());

      if (boost::regex_match(songDescription.c_str(), what, expression) == true)
      {
         found = true;
         screen_.PlaylistWindow().ScrollTo(i + 1);
      }
   }

   return true;
}

bool Search::SearchBackwards(std::string search)
{
   boost::regex  expression(".*" + search + ".*");
   boost::cmatch what;

   bool found = false;

   for (int32_t i = screen_.PlaylistWindow().CurrentLine() - 1; ((i >= 0) && (found == false)); --i)
   {
      std::string songDescription(screen_.PlaylistWindow().GetSong(i)->FullDescription());

      if (boost::regex_match(songDescription.c_str(), what, expression) == true)
      {
         found = true;
         screen_.PlaylistWindow().ScrollTo(i + 1);
      }
   }

   return true;
}

char const * const Search::Prompt() 
{ 
   static char SearchPrompt[] = "/";

   if (direction_ == Forwards)
   {
      SearchPrompt[0] = '/';
   }
   else
   {
      SearchPrompt[0] = '?';
   }

   return SearchPrompt; 
}

bool Search::InputModeHandler(std::string input) 
{
   lastSearch_ = input;
   return NextSearchResult();
}
