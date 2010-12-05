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

   library.cpp - handling of the mpd music library 
   */

#include "library.hpp"

#include "mpdclient.hpp"
#include "screen.hpp"
#include "search.hpp"

using namespace Ui;

LibraryWindow::LibraryWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search) :
   ScrollWindow     (screen),
   settings_        (settings),
   client_          (client),
   search_          (search)
{
}

LibraryWindow::~LibraryWindow()
{
}


void LibraryWindow::AddSong(Mpc::Song const * const song)
{
   if ((song != NULL) && (song->Id() >= 0))
   {
      Mpc::Song * const newSong = new Mpc::Song(*song);
      buffer_.insert(buffer_.end(), newSong);
   }

}


void LibraryWindow::Redraw()
{
   Clear();
   client_.ForEachLibrarySong(*this, &LibraryWindow::AddSong);
}

void LibraryWindow::Clear()
{
   buffer_.clear();
}

void LibraryWindow::Print(uint32_t line) const
{
   static std::string const BlankLine(screen_.MaxColumns(), ' ');

   WINDOW * window = N_WINDOW();

   if (line < buffer_.size())
   {
      mvwprintw(window, line, 0, "%s", buffer_.at(line + FirstLine())->PlaylistDescription().c_str());
   }
}

void LibraryWindow::Confirm() const
{
}

void LibraryWindow::Scroll(int32_t scrollCount)
{
   ScrollWindow::Scroll(scrollCount);
}

void LibraryWindow::ScrollTo(uint16_t scrollLine)
{
   ScrollWindow::ScrollTo(scrollLine);
}
