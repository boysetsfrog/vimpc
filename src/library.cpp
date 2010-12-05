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

#include "colour.hpp"
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
   static std::string artist("");
   static std::string album("");

   if (artist.compare(song->Artist()) != 0)
   {
      LibraryEntry * const libraryEntry = new LibraryEntry();

      libraryEntry->expanded_ = true;
      libraryEntry->str_      = song->Artist();
      libraryEntry->song_     = NULL;
      libraryEntry->type_     = ArtistType;

      buffer_.insert(buffer_.end(), libraryEntry);

      artist = song->Artist();
   }

   if (album.compare(song->Album()) != 0)
   {
      LibraryEntry * const libraryEntry = new LibraryEntry();

      libraryEntry->expanded_ = true;
      libraryEntry->str_      = (song->Album());
      libraryEntry->song_     = NULL;
      libraryEntry->type_     = AlbumType;

      buffer_.insert(buffer_.end(), libraryEntry);

      album = song->Album();
   }

   if ((song != NULL) && (song->Id() >= 0))
   {
      LibraryEntry * const libraryEntry = new LibraryEntry();
      Mpc::Song    * const newSong      = new Mpc::Song(*song);

      libraryEntry->expanded_ = true;
      libraryEntry->str_      = (song->Artist() + "::" + song->Album());
      libraryEntry->song_     = newSong;
      libraryEntry->type_     = SongType;

      buffer_.insert(buffer_.end(), libraryEntry);
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
      if (buffer_.at(line + FirstLine())->type_ == SongType)
      {
         mvwprintw(window, line, 0, "    %s", buffer_.at(line + FirstLine())->song_->Title().c_str());
      }
      else if (buffer_.at(line + FirstLine())->type_ == AlbumType)
      {
         wattron(window, COLOR_PAIR(REDONDEFAULT) | A_BOLD);
         mvwprintw(window, line, 0, "-  %s", buffer_.at(line + FirstLine())->str_.c_str());
         wattroff(window, COLOR_PAIR(REDONDEFAULT) | A_BOLD);
      }
      else if (buffer_.at(line + FirstLine())->type_ == ArtistType)
      {
         wattron(window, COLOR_PAIR(YELLOWONDEFAULT) | A_BOLD);
         mvwprintw(window, line, 0, "- %s", buffer_.at(line + FirstLine())->str_.c_str());
         wattroff(window, COLOR_PAIR(YELLOWONDEFAULT) | A_BOLD);
      }

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
