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
#include "buffer/playlist.hpp"

using namespace Ui;

InfoWindow::InfoWindow(std::string const & URI, Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search, std::string name) :
   SongWindow    (settings, screen, client, clientState, search, name),
   m_URI         (URI)
{
   Redraw();
}

InfoWindow::~InfoWindow()
{
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

      if (song != NULL)
      {
         wattron(window, A_BOLD);
         mvwaddstr(window, 0, 0, " File        : ");
         wattroff(window, A_BOLD);
         wprintw(window, "%s", song->URI().c_str());

         wattron(window, A_BOLD);
         mvwaddstr(window, 3, 0, " Artist      : ");
         wattroff(window, A_BOLD);
         waddstr(window, song->Artist().c_str());

         wattron(window, A_BOLD);
         mvwaddstr(window, 4, 0, " AlbumArtist : ");
         wattroff(window, A_BOLD);
         waddstr(window, song->AlbumArtist().c_str());

         wattron(window, A_BOLD);
         mvwaddstr(window, 5, 0, " Album       : ");
         wattroff(window, A_BOLD);
         waddstr(window, song->Album().c_str());

         wattron(window, A_BOLD);
         mvwaddstr(window, 6, 0, " Track       : ");
         wattroff(window, A_BOLD);
         wprintw(window, "%s", song->Track().c_str());

         wattron(window, A_BOLD);
         mvwaddstr(window, 7, 0, " Title       : ");
         wattroff(window, A_BOLD);
         wprintw(window, "%s", song->Title().c_str());

         wattron(window, A_BOLD);
         mvwaddstr(window, 8, 0, " Duration    : ");
         wattroff(window, A_BOLD);
         wprintw(window, "%d:%.2d", Mpc::SecondsToMinutes(song->Duration()), Mpc::RemainingSeconds(song->Duration()));

         wattron(window, A_BOLD);
         mvwaddstr(window, 9, 0, " Genre       : ");
         wattroff(window, A_BOLD);
         wprintw(window, "%s", song->Genre().c_str());

         wattron(window, A_BOLD);
         mvwaddstr(window, 10, 0, " Date        : ");
         wattroff(window, A_BOLD);
         wprintw(window, "%s", song->Date().c_str());

         wattron(window, A_BOLD);
         mvwaddstr(window, 11, 0, " Disc        : ");
         wattroff(window, A_BOLD);
         wprintw(window, "%s", song->Disc().c_str());

         wattron(window, A_BOLD);
         mvwaddstr(window, 13, 0, " Playlist    : ");
         wattroff(window, A_BOLD);
         wprintw(window, "%s", (song->Reference() > 0) ? "Yes" : "No");

         wattron(window, A_BOLD);
         mvwaddstr(window, 14, 0, " Position    : ");
         wattroff(window, A_BOLD);
         wprintw(window, "%d", Main::Playlist().Index(song) + 1);

         // \TODO rating, counter
      }
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

void InfoWindow::Save(std::string const & name)
{
   SongWindow::Save(name);
}

void InfoWindow::Edit()
{
   int const InfoWindowId = screen_.GetActiveWindow();
   screen_.SetVisible(InfoWindowId, false);
}

void InfoWindow::Redraw()
{
   Clear();

   Mpc::Song * song = Main::Library().Song(m_URI);

   if (song)
   {
      Add(song);
   }
}
/* vim: set sw=3 ts=3: */
