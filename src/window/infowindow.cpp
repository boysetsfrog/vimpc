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

   infowindow.cpp - class representing a scrollable ncurses window
   */

#include "infowindow.hpp"

#include <iostream>

#include "screen.hpp"

using namespace Ui;

InfoWindow::InfoWindow(Mpc::Song * song, Ui::Screen & screen, std::string name) :
   ScrollWindow (screen, name),
   song_        (song)
{
}

InfoWindow::InfoWindow(Ui::Screen & screen, std::string name) :
   ScrollWindow (screen, name),
   song_        (NULL)
{
}

InfoWindow::~InfoWindow()
{
}

void InfoWindow::Print(uint32_t line) const
{
   if ((line == 0) && (song_ != NULL))
   {
      WINDOW * window = N_WINDOW();

      uint32_t printLine = ((screen_.MaxRows()- 5) /2);

      for (unsigned int i = 0; i < 6; ++i)
      {
         mvwhline(window, i, 0, ' ', screen_.MaxColumns());
      }

      wattron(window, A_BOLD);
      mvwaddstr(window, 0, 0, "URI: ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 0, 5, "%s", song_->URI().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, printLine, 0, "Artist: ");
      wattroff(window, A_BOLD);
      mvwaddstr(window, printLine++, 8, song_->Artist().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, printLine, 0, "Album: ");
      wattroff(window, A_BOLD);
      mvwaddstr(window, printLine++, 7, song_->Album().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, printLine, 0, "Track: ");
      wattroff(window, A_BOLD);
      mvwprintw(window, printLine++, 7, "%s - %s", song_->Track().c_str(), song_->Title().c_str());


      //mvwaddstr(window, 4, 0, song_->Duration().c_str());
      //
      // \TODO duration, in playlist? (positions), rating, counter, TAGS -> genre, etc
   }
}
