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

#include "colour.hpp"
#include "mpdclient.hpp"
#include "screen.hpp"

using namespace Ui;

PlaylistWindow::PlaylistWindow(Ui::Screen const & screen, Mpc::Client & client) :
   ScrollWindow     (screen),
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


uint32_t PlaylistWindow::GetCurrentSong() const 
{ 
   return client_.GetCurrentSongId() + 1;
}

uint32_t PlaylistWindow::TotalNumberOfSongs() const 
{ 
   return client_.TotalNumberOfSongs(); 
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
   static std::string const BlankLine(screen_.MaxColumns(), ' ');

   WINDOW * window = N_WINDOW();

   if (line + FirstLine() < BufferSize())
   {
      Mpc::Song * nextSong = buffer_[line + FirstLine()];

      wattron(window, COLOR_PAIR(SONGCOLOUR) | WA_BOLD | WA_BLINK);

      if (line + FirstLine() == currentSelection_)
      {
         wattron(window, A_REVERSE);
      }

      if (nextSong->Id() == GetCurrentSong())
      {
         wattron(window, COLOR_PAIR(CURRENTSONGCOLOUR) | A_BOLD);
      }

      mvwprintw(window, line, 0, BlankLine.c_str());
      wattron(window, A_BOLD);
      mvwprintw(window, line, 0, "[");

      if ((nextSong->Id() != GetCurrentSong()) && (line + FirstLine() != currentSelection_))
      {
         wattron(window, COLOR_PAIR(SONGIDCOLOUR));
      }

      wprintw(window, "%5d", nextSong->Id());

      if ((nextSong->Id() != GetCurrentSong()) && (line + FirstLine() != currentSelection_))
      {
         wattroff(window, COLOR_PAIR(SONGIDCOLOUR));
      }

      wprintw(window, "] ");
      wattroff(window, A_BOLD);

      // \todo make it reprint the current song when
      // it changes and is on screen, this also needs to be done
      // for the status
      if (nextSong->Id() == GetCurrentSong())
      {
         wattron(window, COLOR_PAIR(CURRENTSONGCOLOUR) | WA_BOLD | WA_BLINK);
      }

      wprintw(window, "%s - %s", nextSong->Artist().c_str(), nextSong->Title().c_str());

      std::string const durationString(nextSong->DurationString());
      std::string const albumString   (nextSong->Album());

      //mvwprintw(window, line, (screen_.MaxColumns() - durationString.length() - albumString.length() - 5), "%s   [%s]", nextSong->Album().c_str(), durationString.c_str());
      mvwprintw(window, line, (screen_.MaxColumns() - durationString.length() - 2), "[%s]", durationString.c_str());

      if (nextSong->Id() == GetCurrentSong())
      {
         wattroff(window, COLOR_PAIR(CURRENTSONGCOLOUR) | A_BOLD);
      }

      if (line + FirstLine() == currentSelection_)
      {
         wattroff(window, A_REVERSE);
      }

      wattroff(window, COLOR_PAIR(SONGCOLOUR) | A_BOLD | A_BLINK);
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
      ScrollWindow::Scroll(scrollCount);
   }
}

void PlaylistWindow::ScrollTo(uint16_t scrollLine)
{
   int64_t oldSelection = currentSelection_;
   currentSelection_    = ((int64_t) scrollLine - 1);
   currentSelection_    = LimitCurrentSelection(currentSelection_);

   if ((currentSelection_ == scrollLine_) && (currentSelection_ - oldSelection == 1))
   {
      ScrollWindow::Scroll(1);
   }
   else if ((currentSelection_ == scrollLine_ - screen_.MaxRows() - 1) && (currentSelection_ - oldSelection == -1))
   {
      ScrollWindow::Scroll(-1);
   }
   else if ((currentSelection_ >= scrollLine_) || (currentSelection_ < scrollLine_ - screen_.MaxRows()))
   {   
      ScrollWindow::ScrollTo(scrollLine);
   }
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
