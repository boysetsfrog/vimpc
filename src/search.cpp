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

char const SearchPrompt   = '/';

Search::Search(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings) :
   InputMode   (SearchPrompt, screen),
   Player      (screen, client, settings),
   settings_   (settings),
   screen_     (screen)
{
}

Search::~Search()
{
}

bool Search::InputModeHandler(std::string input)
{
   boost::regex  expression(".*" + input + ".*");
   boost::cmatch what;

   bool found = false;

   for (uint32_t i = screen_.PlaylistWindow().FirstLine(); ((i <= screen_.PlaylistWindow().LastLine()) && (found == false)); ++i)
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
