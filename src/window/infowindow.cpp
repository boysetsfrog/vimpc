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

#include "mpdclient.hpp"
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

      for (unsigned int i = 0; i < screen_.MaxRows(); ++i)
      {
         mvwhline(window, i, 0, ' ', screen_.MaxColumns());
      }

      wattron(window, A_BOLD);
      mvwaddstr(window, 0, 0, " URI: ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 0, 6, "%s", song_->URI().c_str());


      wattron(window, A_BOLD);
      mvwaddstr(window, 3, 0, " Artist   : ");
      wattroff(window, A_BOLD);
      mvwaddstr(window, 3, 12, song_->Artist().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, 4, 0, " Album    : ");
      wattroff(window, A_BOLD);
      mvwaddstr(window, 4, 12, song_->Album().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, 5, 0, " Track    : ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 5, 12, "%s - %s", song_->Track().c_str(), song_->Title().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, 6, 0, " Duration : ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 6, 12, "%d:%d", Mpc::SecondsToMinutes(song_->Duration()), Mpc::RemainingSeconds(song_->Duration()));


      wattron(window, A_BOLD);
      mvwaddstr(window, 8, 0, " In Playlist : ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 8, 15, "%s", (song_->Reference() > 0) ? "Yes" : "No");

      //mvwaddstr(window, 4, 0, song_->Duration().c_str());
      //
      // \TODO playlist positions, rating, counter, other TAGS (genre, date)
   }
}
