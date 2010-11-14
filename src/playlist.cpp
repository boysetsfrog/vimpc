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

   playlist.cpp - handling of the mpd playlist interface 
   */

#include "playlist.hpp"

#include "mpdclient.hpp"
#include "screen.hpp"

using namespace Ui;

PlaylistWindow::PlaylistWindow(Ui::Screen const & screen, Mpc::Client & client) :
   Window           (screen),
   currentSelection_(0),
   client_          (client)
{
   // \todo class requires a lot of cleaning up
}

PlaylistWindow::~PlaylistWindow()
{
   DeleteSongs();   
}


void PlaylistWindow::AddSong(Mpc::Song const * const song)
{
   if ((song != NULL) && (song->Id() >= 0))
   {
      Mpc::Song * const newSong = new Mpc::Song(*song);
      buffer_.insert(buffer_.end(), newSong);
   }
}

Mpc::Song const * const PlaylistWindow::GetSong(uint32_t songIndex)
{
   Mpc::Song const * song = NULL;

   if (songIndex < buffer_.size())
   {
      song = buffer_.at(songIndex);
   }

   return song; 
}

void PlaylistWindow::Redraw()
{
   Clear();

   client_.ForEachSong(*this, &PlaylistWindow::AddSong);
}

void PlaylistWindow::Clear()
{
   DeleteSongs();
   buffer_.clear();
}

void PlaylistWindow::DeleteSongs()
{
   for (SongBuffer::iterator it = buffer_.begin(); (it != buffer_.end()); ++it)
   {
      delete *it;
   }
}

void PlaylistWindow::Print(uint32_t line) const
{
   std::string const BlankLine(screen_.MaxColumns(), ' ');

   WINDOW * window = N_WINDOW();

   // \todo make a library file for color definitions
   init_pair(COLOR_PAIRS-1, COLOR_YELLOW, COLOR_BLACK);
   init_pair(COLOR_PAIRS-2, COLOR_BLUE, COLOR_BLACK);

   if (line + FirstLine() < BufferSize())
   {
      Mpc::Song * nextSong = buffer_[line + FirstLine()];

      if (line + FirstLine() == currentSelection_)
      {
         wattron(window, A_REVERSE);
      }

      if (nextSong->Id() == client_.GetCurrentSong() + 1)
      {
         wattron(window, A_BOLD);
         wattron(window, COLOR_PAIR(COLOR_PAIRS-2));
      }

      mvwprintw(window, line, 0, BlankLine.c_str());
      wattron(window, A_BOLD);
      mvwprintw(window, line, 0, "[");

      if (nextSong->Id() != client_.GetCurrentSong() + 1)
      {
         wattron(window, COLOR_PAIR(COLOR_PAIRS-1));
      }

      wprintw(window, "%4d", nextSong->Id());

      if (nextSong->Id() != client_.GetCurrentSong() + 1)
      {
         wattroff(window, COLOR_PAIR(COLOR_PAIRS-1));
      }

      wprintw(window, "] ");
      wattroff(window, A_BOLD);

      // \todo make it reprint the current song when
      // it changes and is on screen, this also needs to be done
      // for the status
      // \todo maybe make the current song do the +1
      if (nextSong->Id() == client_.GetCurrentSong() + 1)
      {
         wattron(window, A_BOLD);
         wattron(window, COLOR_PAIR(COLOR_PAIRS-2));
      }

      wprintw(window, "%s - %s", nextSong->Artist().c_str(), nextSong->Title().c_str());

      if (nextSong->Id() == client_.GetCurrentSong() + 1)
      {
         wattroff(window, A_BOLD);
         wattroff(window, COLOR_PAIR(COLOR_PAIRS-2));
      }

      if (line + FirstLine() == currentSelection_)
      {
         wattroff(window, A_REVERSE);
      }
   }
}

void PlaylistWindow::Confirm() const
{
   client_.Play((uint32_t) currentSelection_);
}

void PlaylistWindow::Scroll(int32_t scrollCount)
{
   currentSelection_ += scrollCount;
   currentSelection_  = LimitCurrentSelection(currentSelection_);

   if ((currentSelection_ >= scrollLine_) || (currentSelection_ < scrollLine_ - screen_.MaxRows()))
   {   
      Window::Scroll(scrollCount);
   }
}

void PlaylistWindow::ScrollTo(uint16_t scrollLine)
{
   currentSelection_ = ((int64_t) scrollLine - 1);
   currentSelection_ = LimitCurrentSelection(currentSelection_);

   if (currentSelection_ == scrollLine_)
   {
      Window::Scroll(1);
   }
   else if (currentSelection_ == scrollLine_ - screen_.MaxRows() - 1)
   {
      Window::Scroll(-1);
   }
   else if ((currentSelection_ >= scrollLine_) || (currentSelection_ < scrollLine_ - screen_.MaxRows()))
   {   
      Window::ScrollTo(scrollLine);
   }
}

void PlaylistWindow::Search(std::string const & searchString) const
{
   
}

int64_t PlaylistWindow::LimitCurrentSelection(int64_t currentSelection) const
{
   if (currentSelection < 0)
   {
      currentSelection = 0;
   }
   else if (currentSelection_ >= BufferSize())
   {
      currentSelection = BufferSize() - 1;
   }

   return currentSelection;
}
