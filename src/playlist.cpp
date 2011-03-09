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
#include "search.hpp"
#include "settings.hpp"
#include "screen.hpp"

#include <boost/regex.hpp>

using namespace Ui;

PlaylistWindow::PlaylistWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (screen),
   settings_        (settings),
   client_          (client),
   search_          (search),
   buffer_          ()
{
   // \todo class requires a lot of cleaning up
}

PlaylistWindow::~PlaylistWindow()
{
   DeleteSongs();   
}


void PlaylistWindow::AddSong(Mpc::Song const * const song)
{
   if (song != NULL)
   {
      Mpc::Song * const newSong = new Mpc::Song(*song);
      buffer_.insert(buffer_.end(), newSong);
   }
}

Mpc::Song const * PlaylistWindow::GetSong(uint32_t songIndex)
{
   Mpc::Song const * song = NULL;

   if (songIndex < buffer_.size())
   {
      song = buffer_.at(songIndex);
   }

   return song; 
}

void PlaylistWindow::RemoveSong(uint32_t count)
{
   SongBuffer::iterator it = buffer_.begin(); 

   for (uint32_t id = 0; id != CurrentLine(); ++id)
   {
      it++;
   }

   for(uint32_t i = count; i > 0; --i)
   {
      //! \todo delete invalidates the playlist id numbers, this needs to be fixed
      Mpc::Song const * song = GetSong(CurrentLine());

      client_.Delete(CurrentLine());

      if (it != buffer_.end())
      {
         it = buffer_.erase(it);
      }

      delete song;
   }

   currentSelection_ = LimitCurrentSelection(currentSelection_);
}


uint32_t PlaylistWindow::GetCurrentSong() const 
{ 
   return client_.GetCurrentSong();
}

uint32_t PlaylistWindow::TotalNumberOfSongs() const 
{ 
   return client_.TotalNumberOfSongs(); 
}


void PlaylistWindow::Redraw()
{
   uint16_t currentLine = CurrentLine();
   uint16_t scrollLine  = ScrollLine();

   Clear();

   client_.ForEachQueuedSong(*this, &PlaylistWindow::AddSong);

   SetScrollLine(scrollLine);
   ScrollTo(++currentLine);
}

void PlaylistWindow::Print(uint32_t line) const
{
   uint32_t printLine = line + FirstLine();

   if (printLine < BufferSize())
   {
      WINDOW    * window      = N_WINDOW();
      Mpc::Song * nextSong    = buffer_[printLine];
      int32_t     colour      = DetermineSongColour(line + FirstLine(), nextSong);
      
      wattron(window, COLOR_PAIR(colour) | A_BOLD);

      if (printLine == CurrentLine())
      {
         wattron(window, A_REVERSE);
      }
      else if (colour != CURRENTSONGCOLOUR)
      {
         wattron(window, COLOR_PAIR(SONGCOLOUR));
      }

      mvwhline(window,  line, 0, ' ', screen_.MaxColumns());
      mvwaddstr(window, line, 0, "[");

      if ((colour != CURRENTSONGCOLOUR) && (printLine != CurrentLine()))
      {
         wattron(window, COLOR_PAIR(SONGIDCOLOUR));
      }

      wprintw(window, "%5d", nextSong->Id());

      if ((colour != CURRENTSONGCOLOUR) && (printLine != CurrentLine()))
      {
         wattroff(window, COLOR_PAIR(SONGIDCOLOUR));
      }

      waddstr(window, "] ");

      if (colour != CURRENTSONGCOLOUR)
      {
         wattroff(window, A_BOLD);
      }

      wattron(window, COLOR_PAIR(colour));

      std::string artist = nextSong->Artist().c_str();
      std::string title  = nextSong->Title().c_str();

      if (title == "Unknown")
      {
         title = nextSong->URI().c_str();
      }

      wprintw(window, "%s - %s", artist.c_str(), title.c_str());

      if ((colour != CURRENTSONGCOLOUR) && (printLine != CurrentLine()))
      {
         wattron(window, COLOR_PAIR(SONGCOLOUR));
      }

      std::string const durationString(nextSong->DurationString());
      mvwprintw(window, line, (screen_.MaxColumns() - durationString.size() - 2), "[%s]", durationString.c_str());

      wattroff(window, A_BOLD | A_REVERSE | COLOR_PAIR(colour));
      wredrawln(window, line, 1);
   }
}

void PlaylistWindow::Left(Ui::Player & player, uint32_t count)
{
   player.SkipSong(Ui::Player::Previous, count);
}

void PlaylistWindow::Right(Ui::Player & player, uint32_t count)
{
   player.SkipSong(Ui::Player::Next, count);
}

void PlaylistWindow::Confirm()
{
   Mpc::Song const * const song = buffer_.at(CurrentLine());
   client_.Play(static_cast<uint32_t>(song->Id() - 1));
}


int32_t PlaylistWindow::DetermineSongColour(uint32_t line, Mpc::Song const * const nextSong) const
{
   int32_t colour = SONGCOLOUR;

   if (line == GetCurrentSong())
   {
      colour = CURRENTSONGCOLOUR;
   }
   else if ((search_.LastSearchString() != "") && (settings_.HightlightSearch() == true))
   {
      boost::regex expression(".*" + search_.LastSearchString() + ".*");

      if (boost::regex_match(nextSong->PlaylistDescription(), expression))
      {
         colour = SONGMATCHCOLOUR;
      } 
   }

   return colour;
}

void PlaylistWindow::DeleteSongs()
{
   for (SongBuffer::iterator it = buffer_.begin(); (it != buffer_.end()); ++it)
   {
      delete *it;
   }
}

void PlaylistWindow::Clear()
{
   ScrollTo(0);

   DeleteSongs();
   buffer_.clear();
}
