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

#include <iostream>

#include "vimpc.hpp"
#include "settings.hpp"

using namespace Ui;

char const SearchPrompt   = '/';

Search::Search(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings) :
   InputMode   (SearchPrompt, screen),
   Player      (screen, client),
   settings_   (settings),
   screen_     (screen)
{
}

Search::~Search()
{
}

bool Search::InputModeHandler(std::string input)
{
   screen_.Search(input);

   return true;
}
