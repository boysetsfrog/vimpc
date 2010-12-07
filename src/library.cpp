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

#include <iostream>

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
   //! \todo delete the ridiculous amount of newed objects
}


void LibraryWindow::AddSong(Mpc::Song const * const song)
{
   //! \todo could use a massive refactor, there is lots of repeated code here

   if (library_.find(song->Artist()) == library_.end())
   {
      LibraryHeirachyEntry * const entry = new LibraryHeirachyEntry();

      entry->libraryEntry_.expanded_ = false;
      entry->libraryEntry_.str_      = song->Artist();
      entry->libraryEntry_.song_     = NULL;
      entry->libraryEntry_.type_     = ArtistType;

      library_[song->Artist()] = entry;
   }

   LibraryHeirachyEntry * const artistEntry = library_.find(song->Artist())->second;

   if (artistEntry->children_.find(song->Album()) == artistEntry->children_.end())
   {
      LibraryHeirachyEntry * const entry = new LibraryHeirachyEntry();

      entry->libraryEntry_.expanded_ = false;
      entry->libraryEntry_.str_      = (song->Album());
      entry->libraryEntry_.song_     = NULL;
      entry->libraryEntry_.type_     = AlbumType;

      artistEntry->children_[song->Album()] = entry;
   }

   LibraryHeirachyEntry * const albumEntry = (artistEntry->children_.find(song->Album()))->second;

   if ((song != NULL) && (song->Id() >= 0))
   {
      LibraryHeirachyEntry * const entry = new LibraryHeirachyEntry();
      Mpc::Song * const newSong      = new Mpc::Song(*song);

      entry->libraryEntry_.expanded_ = false;
      entry->libraryEntry_.str_      = (song->Artist() + "::" + song->Album());
      entry->libraryEntry_.song_     = newSong;
      entry->libraryEntry_.type_     = SongType;

      albumEntry->children_[song->Title()] = entry;
   }

}


void LibraryWindow::Redraw()
{
   Clear();
   client_.ForEachLibrarySong(*this, &LibraryWindow::AddSong);
   PopulateBuffer();
}

void LibraryWindow::Clear()
{
   buffer_.clear();
}

void LibraryWindow::PopulateBuffer()
{
   //! \todo need to delete library heirachy objects first
   Clear();

   //! \todo try and make this a bit more efficient, two data structures is a bit dodgy
   //        and will almost certainly be quite slow
   for (Library::iterator artistIt = library_.begin(); artistIt != library_.end(); ++artistIt)
   {
      buffer_.insert(buffer_.end(), &(artistIt->second->libraryEntry_)); 

      if (artistIt->second->libraryEntry_.expanded_ == true)
      {
         for (Library::iterator albumIt = artistIt->second->children_.begin(); albumIt != artistIt->second->children_.end(); ++albumIt)
         {
            buffer_.insert(buffer_.end(), &(albumIt->second->libraryEntry_)); 

            if (albumIt->second->libraryEntry_.expanded_ == true)
            {
               for (Library::iterator songIt = albumIt->second->children_.begin(); songIt != albumIt->second->children_.end(); ++songIt)
               {
                  buffer_.insert(buffer_.end(), &(songIt->second->libraryEntry_)); 
               }
            }
         }
      }
   }
}

void LibraryWindow::Print(uint32_t line) const
{
   static std::string const BlankLine(screen_.MaxColumns(), ' ');
   static uint32_t          offsetCount = 0;

   if (line == 0)
   {
      offsetCount = 0;
   }

   WINDOW * window = N_WINDOW();

   if ((line + offsetCount) < buffer_.size())
   {
      uint32_t printLine = (line + offsetCount + FirstLine());

      char expand = (buffer_.at(printLine)->expanded_ == true) ? '-' : '+';

      if (buffer_.at(printLine)->type_ == SongType)
      {
         mvwprintw(window, line, 0, "    %s", buffer_.at(printLine)->song_->Title().c_str());
      }
      else if (buffer_.at(printLine)->type_ == AlbumType)
      {
         wattron(window, COLOR_PAIR(REDONDEFAULT) | A_BOLD);
         mvwprintw(window, line, 0, "%c  %s", expand, buffer_.at(printLine)->str_.c_str());
         wattroff(window, COLOR_PAIR(REDONDEFAULT) | A_BOLD);
      }
      else if (buffer_.at(printLine)->type_ == ArtistType)
      {
         wattron(window, COLOR_PAIR(YELLOWONDEFAULT) | A_BOLD);
         mvwprintw(window, line, 0, "%c %s", expand, buffer_.at(printLine)->str_.c_str());
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
