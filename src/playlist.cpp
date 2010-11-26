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
#include "console.hpp"
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
   ScrollTo(0);

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
   static uint32_t currentSong = 0;

   if (line == 0)
   {
      //Saves us from querying mpd all the time
      // \todo will eventually cache this in mpd client for a second at a time
      //       when that is done return to setting this for each print (will no longer need to be static)
      currentSong = GetCurrentSong();
   }

   WINDOW * window = N_WINDOW();

   if (line + FirstLine() < BufferSize())
   {
      Mpc::Song * nextSong = buffer_[line + FirstLine()];

      wattron(window, (nextSong->Id() != currentSong) ? COLOR_PAIR(SONGCOLOUR) : COLOR_PAIR(CURRENTSONGCOLOUR));

      if (line + FirstLine() == currentSelection_)
      {
         wattron(window, A_REVERSE);
      }

      wattron(window, A_BOLD);
      mvwhline(window,  line, 0, ' ', screen_.MaxColumns());
      mvwaddstr(window, line, 0, "[");

      if ((nextSong->Id() != currentSong) && (line + FirstLine() != currentSelection_))
      {
         wattron(window, COLOR_PAIR(SONGIDCOLOUR));
      }

      wprintw(window, "%5d", nextSong->Id());

      if ((nextSong->Id() != currentSong) && (line + FirstLine() != currentSelection_))
      {
         wattroff(window, COLOR_PAIR(SONGIDCOLOUR));
      }

      waddstr(window, "] ");

      // \todo make it reprint the current song when
      // it changes and is on screen, this also needs to be done
      // for the status
      if (nextSong->Id() != currentSong)
      {
         wattroff(window, A_BOLD);
      }

      std::string const durationString(nextSong->DurationString());

      waddstr(window, nextSong->Artist().c_str());
      waddstr(window, " - ");
      waddstr(window, nextSong->Title().c_str());

      wmove(window, line, (screen_.MaxColumns() - durationString.size() - 2));
      wprintw(window, "[%s]", durationString.c_str());

      wattroff(window, A_BOLD | A_REVERSE);
      wattroff(window, (nextSong->Id() != currentSong) ? COLOR_PAIR(SONGCOLOUR) : COLOR_PAIR(CURRENTSONGCOLOUR));
      wredrawln(window, line, 1);
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
