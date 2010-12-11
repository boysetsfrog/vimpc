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

#include <algorithm>
#include <iostream>

using namespace Ui;

LibraryWindow::LibraryWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (screen),
   settings_        (settings),
   client_          (client),
   search_          (search),
   library_         (),
   buffer_          ()
{
   library_ = new Library();
}

LibraryWindow::~LibraryWindow()
{
   delete library_;
   //! \todo delete the ridiculous amount of newed objects
}


void LibraryWindow::AddSong(Mpc::Song const * const song)
{
   //! \todo could use a massive refactor, there is lots of repeated code here
   static uint32_t nextSong = 0;

   std::string artist = song->Artist();
   std::transform(artist.begin(), artist.end(), artist.begin(), ::toupper);

   std::string album = song->Album();
   std::transform(album.begin(), album.end(), album.begin(), ::toupper);

   if (library_->find(artist) == library_->end())
   {
      nextSong = 0;

      LibraryHeirachyEntry * const entry = new LibraryHeirachyEntry();

      entry->libraryEntry_.artist_   = song->Artist();
      entry->libraryEntry_.type_     = ArtistType;

      (*library_)[artist] = entry;
   }

   LibraryHeirachyEntry * const artistEntry = library_->find(artist)->second;

   if (artistEntry->children_.find(album) == artistEntry->children_.end())
   {
      nextSong = 0;

      LibraryHeirachyEntry * const entry = new LibraryHeirachyEntry();

      entry->libraryEntry_.artist_   = song->Artist();
      entry->libraryEntry_.album_    = song->Album();
      entry->libraryEntry_.type_     = AlbumType;

      artistEntry->children_[album] = entry;
   }


   LibraryHeirachyEntry * const albumEntry = (artistEntry->children_.find(album))->second;

   if (song != NULL)
   {
      char songIdBuffer[32];

      LibraryHeirachyEntry * const entry = new LibraryHeirachyEntry();
      Mpc::Song * const newSong      = new Mpc::Song(*song);

      entry->libraryEntry_.expanded_ = false;
      entry->libraryEntry_.artist_   = song->Artist();
      entry->libraryEntry_.album_    = song->Album();
      entry->libraryEntry_.song_     = newSong;
      entry->libraryEntry_.type_     = SongType;

      snprintf(songIdBuffer, 32, "%d", nextSong++);

      std::string songId(songIdBuffer);

      albumEntry->children_[songId] = entry;
   }

}


void LibraryWindow::Redraw()
{
   Clear();
   client_.ForEachLibrarySong(*this, &LibraryWindow::AddSong);
   PopulateBuffer();
}

void LibraryWindow::Expand(uint32_t line)
{
   if (buffer_.at(line)->expanded_ == false)
   {
      buffer_.at(line)->expanded_ = true;

      SongBuffer::iterator position = buffer_.begin();

      for (uint32_t i = 0; i <= line; ++i)
      {
         ++position;
      }

      if (buffer_.at(line)->type_ == ArtistType)
      {
         std::string searchString = buffer_.at(line)->artist_;
         std::transform(searchString.begin(), searchString.end(),searchString.begin(), ::toupper);

         Library::iterator it = library_->find(searchString);

         if (it != library_->end())
         {

            for (Library::reverse_iterator children = it->second->children_.rbegin(); children != it->second->children_.rend(); ++children)
            {
               position = buffer_.insert(position, &(children->second->libraryEntry_));
            }
         }
      }
      
      if (buffer_.at(line)->type_ == AlbumType)
      {
         std::string searchString = buffer_.at(line)->artist_;
         std::transform(searchString.begin(), searchString.end(), searchString.begin(), ::toupper);

         std::string searchString2 = buffer_.at(line)->album_;
         std::transform(searchString2.begin(), searchString2.end(), searchString2.begin(), ::toupper);

         Library::iterator it = library_->find(searchString);

         if (it != library_->end())
         {
            Library::iterator it2 = it->second->children_.find(searchString2);

            for (Library::reverse_iterator children = it2->second->children_.rbegin(); children != it2->second->children_.rend(); ++children)
            {
               position = buffer_.insert(position, &(children->second->libraryEntry_));
            }
         }
      }

   }
}

void LibraryWindow::Clear()
{
   buffer_.clear();

   for (Library::iterator it = library_->begin(); it != library_->end(); ++it)
   {
      for (Library::iterator it2 = it->second->children_.begin(); it2 != it->second->children_.end(); ++it2)
      {
         for (Library::iterator it3 = it2->second->children_.begin(); it3 != it2->second->children_.end(); ++it3)
         {
            delete it3->second->libraryEntry_.song_;
            delete it3->second;
         }

         it2->second->children_.clear();
         delete it2->second;
      }
      
      it->second->children_.clear();
      delete it->second;
   }

   library_->clear();

}

void LibraryWindow::PopulateBuffer()
{
   //! \todo try and make this a bit more efficient, two data structures is a bit dodgy
   //        and will almost certainly be quite slow
   for (Library::iterator artistIt = library_->begin(); artistIt != library_->end(); ++artistIt)
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

      if (FirstLine() + line == CurrentLine())
      {
         wattron(window, A_REVERSE);
      }

      if ((buffer_.at(printLine)->type_ == SongType) && (buffer_.at(printLine)->song_ != NULL))
      {
         mvwprintw(window, line, 0, BlankLine.c_str());
         mvwprintw(window, line, 0, "    %s", buffer_.at(printLine)->song_->Title().c_str());
      }
      else if (buffer_.at(printLine)->type_ == AlbumType)
      {
         wattron(window, COLOR_PAIR(REDONDEFAULT) | A_BOLD);
         mvwprintw(window, line, 0, BlankLine.c_str());
         mvwprintw(window, line, 0, "%c  %s", expand, buffer_.at(printLine)->album_.c_str());
         wattroff(window, COLOR_PAIR(REDONDEFAULT) | A_BOLD);
      }
      else if (buffer_.at(printLine)->type_ == ArtistType)
      {
         wattron(window, COLOR_PAIR(YELLOWONDEFAULT) | A_BOLD);
         mvwprintw(window, line, 0, BlankLine.c_str());
         mvwprintw(window, line, 0, "%c %s", expand, buffer_.at(printLine)->artist_.c_str());
         wattroff(window, COLOR_PAIR(YELLOWONDEFAULT) | A_BOLD);
      }

      wattroff(window, A_REVERSE);
   }
}

void LibraryWindow::Confirm()
{
   Expand(CurrentLine());
}
