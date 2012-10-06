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

InfoWindow::InfoWindow(Mpc::Song * song, Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Ui::Search const & search, std::string name) :
   SongWindow    (settings, screen, client, search, name),
   m_ActiveWindow(screen_.GetActiveWindow()),
   m_Song        (new Mpc::Song(*song))
{
   Add(m_Song);
}

InfoWindow::~InfoWindow()
{
   delete m_Song;
}

void InfoWindow::Print(uint32_t line) const
{
   if (line == 0)
   {
      WINDOW * window = N_WINDOW();
      Mpc::Song * song = Buffer().Get(line);

      for (unsigned int i = 0; i < screen_.MaxRows(); ++i)
      {
         mvwhline(window, i, 0, ' ', screen_.MaxColumns());
      }

      wattron(window, A_BOLD);
      mvwaddstr(window, 0, 0, " File     : ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 0, 12, "%s", song->URI().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, 3, 0, " Artist   : ");
      wattroff(window, A_BOLD);
      mvwaddstr(window, 3, 12, song->Artist().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, 4, 0, " Album    : ");
      wattroff(window, A_BOLD);
      mvwaddstr(window, 4, 12, song->Album().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, 5, 0, " Track    : ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 5, 12, "%s", song->Track().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, 6, 0, " Title    : ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 6, 12, "%s", song->Title().c_str());

      wattron(window, A_BOLD);
      mvwaddstr(window, 7, 0, " Duration : ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 7, 12, "%d:%d", Mpc::SecondsToMinutes(song->Duration()), Mpc::RemainingSeconds(song->Duration()));


      wattron(window, A_BOLD);
      mvwaddstr(window, 9, 0, " Playlist : ");
      wattroff(window, A_BOLD);
      mvwprintw(window, 9, 12, "%s", (song->Reference() > 0) ? "Yes" : "No");

      //mvwaddstr(window, 4, 0, song->Duration().c_str());
      //
      // \TODO playlist positions, rating, counter, other TAGS (genre, date)
   }
}


void InfoWindow::AddLine(uint32_t line, uint32_t count, bool scroll)
{
   SongWindow::AddLine(0, 1, false);
}

void InfoWindow::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
   SongWindow::DeleteLine(0, 1, false);
}

void InfoWindow::Edit()
{
   int const InfoWindowId = screen_.GetActiveWindow();

   if (screen_.IsVisible(m_ActiveWindow) == true)
   {
      screen_.SetActiveAndVisible(m_ActiveWindow);
   }

   screen_.SetVisible(InfoWindowId, false);
}
/* vim: set sw=3 ts=3: */
