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
#include "settings.hpp"

#include <algorithm>
#include <boost/regex.hpp>
#include <iostream>

using namespace Ui;

//! \todo this entire class needs massive refactoring
//
//! \todo the tracks are not sorted properly within the albums

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
   Clear();
   delete library_;
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


std::string LibraryWindow::SearchPattern(int32_t id)
{
   //! \todo add a search that searches in collapsed songs and
   //! expands things as necessary
   std::string pattern("");

   switch (buffer_.at(id)->type_)
   {
      case ArtistType:
         pattern = buffer_.at(id)->artist_;
         break;
      
      case AlbumType:
         pattern = buffer_.at(id)->album_;
         break;

      case SongType:
         pattern = buffer_.at(id)->song_->Title();
         break;

      default:
         ASSERT(false);
         break;
   }

   return pattern;
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

void LibraryWindow::Collapse(uint32_t line)
{
   if ((buffer_.at(line)->expanded_ == false) && (buffer_.at(line)->type_ == ArtistType))
   {
      Scroll(-1);
   }
   else
   {
      EntryType endType = SongType;

      if ((buffer_.at(line)->type_ == SongType) || ((buffer_.at(line)->type_ == AlbumType) && (buffer_.at(line)->expanded_ == true)))
      {
         while (buffer_.at(line)->type_ == SongType)
         {
            --currentSelection_;
            --line;
         }

         endType = AlbumType;
      }
      else if ((buffer_.at(line)->type_ == AlbumType) || ((buffer_.at(line)->type_ == ArtistType) && (buffer_.at(line)->expanded_ == true)))
      {
         while (buffer_.at(line)->type_ != ArtistType)
         {
            --currentSelection_;
            --line;
         }

         endType = ArtistType;
      }

      if (buffer_.at(line)->expanded_ == true)
      {
         buffer_.at(line)->expanded_ = false;
      }

      ++line;

      SongBuffer::iterator position = buffer_.begin();

      for (uint32_t i = 0; i < line; ++i)
      {
         ++position;
      }

      if (endType == AlbumType)
      {
         while ((position != buffer_.end()) && (buffer_.at(line)->type_ == SongType))
         {
            buffer_.at(line)->expanded_ = false;
            position = buffer_.erase(position);
         }
      }
      else if (endType == ArtistType)
      {
         while ((position != buffer_.end()) && ((buffer_.at(line)->type_ == SongType) || (buffer_.at(line)->type_ == AlbumType)))
         {
            buffer_.at(line)->expanded_ = false;
            position = buffer_.erase(position);
         }
      }

      if (FirstLine() + screen_.MaxRows() - 1 > BufferSize())
      {
         scrollLine_ = BufferSize();
      }

      if (scrollLine_ > currentSelection_)
      {
         ScrollTo(currentSelection_ + 1);
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
   //! \todo make songs that are currently in the playlist display a different colour?
   static std::string const BlankLine(screen_.MaxColumns(), ' ');
   static int32_t trackCount = 0;

   WINDOW * window = N_WINDOW();

   if (line < buffer_.size())
   {
      uint32_t printLine = (line + FirstLine());

      int colour = DetermineSongColour(buffer_.at(printLine));

      if (printLine == CurrentLine())
      {
         wattron(window, COLOR_PAIR(colour));
         wattron(window, A_REVERSE);
      }

      mvwprintw(window, line, 0, BlankLine.c_str());

      if ((buffer_.at(printLine)->type_ == AlbumType) || (buffer_.at(printLine)->type_ == ArtistType))
      {
         trackCount = 0;
         uint8_t expandCol = 0;

         if (buffer_.at(printLine)->type_ == AlbumType)
         {
            mvwprintw(window, line, 1, "|--");
            expandCol = 4;
         }
         
         mvwprintw(window, line, expandCol, "[ ]");

         if (printLine != CurrentLine()) 
         { 
            wattron(window, COLOR_PAIR(REDONDEFAULT) | A_BOLD); 
         }
         
         char expand = (buffer_.at(printLine)->expanded_ == true) ? '-' : '+';
         mvwprintw(window, line, expandCol + 1, "%c", expand);

         if (printLine != CurrentLine()) 
         { 
            wattroff(window, COLOR_PAIR(REDONDEFAULT) | A_BOLD); 
         }

         wattron(window, A_BOLD | COLOR_PAIR(colour));
         wmove(window, line, expandCol + 4);

         if (buffer_.at(printLine)->type_ == ArtistType)
         {
            waddstr(window, buffer_.at(printLine)->artist_.c_str());
         }
         else if (buffer_.at(printLine)->type_ == AlbumType)
         {
            waddstr(window, buffer_.at(printLine)->album_.c_str());
         }

         wattroff(window, A_BOLD | COLOR_PAIR(colour));
      }
      else if ((buffer_.at(printLine)->type_ == SongType) && (buffer_.at(printLine)->song_ != NULL))
      {
         trackCount++;
         mvwprintw(window, line, 1, "|   |-- ");

         wattron(window, A_BOLD);
         wprintw(window, "%2d", trackCount);
         wattroff(window, A_BOLD);

         waddstr(window, ". ");
         wattron(window, COLOR_PAIR(colour));

         std::string title = buffer_.at(printLine)->song_->Title();

         if (title == "Unknown")
         {
            title = buffer_.at(printLine)->song_->URI();
         }

         if (title.length() >= 48)
         {
            title  = title.substr(0, 45);
            title += "...";
         }

         waddstr(window, title.c_str());

         mvwprintw(window, line, 61, " [");
         waddstr(window, buffer_.at(printLine)->song_->DurationString().c_str());
         waddstr(window, "]");
      }

      wattroff(window, A_REVERSE | COLOR_PAIR(colour));
   }
}

void LibraryWindow::Left(UNUSED Ui::Player & player, UNUSED uint32_t count)
{
   Collapse(CurrentLine());
}

void LibraryWindow::Right(UNUSED Ui::Player & player, UNUSED uint32_t count)
{
   Expand(CurrentLine());
   Scroll(1);
}

void LibraryWindow::Confirm()
{
   client_.Clear();
   
   int32_t song = AddSongs();

   if (song != -1)
   {
      client_.Play(song);
   }
}

int32_t LibraryWindow::AddSongs()
{
   int32_t firstSong = -1;

   if (buffer_.at(CurrentLine())->type_ == SongType)
   {
      if (buffer_.at(CurrentLine())->song_ != NULL)
      {
         firstSong = client_.Add(*(buffer_.at(CurrentLine())->song_));
      }
   }

   else if (buffer_.at(CurrentLine())->type_ == ArtistType)
   {
      std::string searchString = buffer_.at(CurrentLine())->artist_;
      std::transform(searchString.begin(), searchString.end(),searchString.begin(), ::toupper);

      Library::iterator it = library_->find(searchString);

      Library const * const library = &(it->second->children_);

      for (Library::const_iterator it2 = library->begin(); it2 != library->end(); ++it2)
      {
         Library const * const library2 = &(it2->second->children_);

         for (Library::const_iterator it3 = library2->begin(); it3 != library2->end(); ++it3)
         {
            int32_t added = client_.Add(*(it3->second->libraryEntry_.song_));

            if (firstSong == -1)
            {
               firstSong = added;
            }
         }
      }
   }

   else if (buffer_.at(CurrentLine())->type_ == AlbumType)
   {
      std::string searchString = buffer_.at(CurrentLine())->artist_;
      std::transform(searchString.begin(), searchString.end(),searchString.begin(), ::toupper);

      std::string searchString2 = buffer_.at(CurrentLine())->album_;
      std::transform(searchString2.begin(), searchString2.end(), searchString2.begin(), ::toupper);

      Library::iterator it = library_->find(searchString);

      Library const * const library = &(it->second->children_);

      Library::const_iterator it2 = library->find(searchString2);

      Library const * const library2 = &(it2->second->children_);

      for (Library::const_iterator it3 = library2->begin(); it3 != library2->end(); ++it3)
      {
         int32_t added = client_.Add(*(it3->second->libraryEntry_.song_));

         if (firstSong == -1)
         {
            firstSong = added;
         }
      }
   }

   return firstSong;
}

int32_t LibraryWindow::DetermineSongColour(LibraryEntry const * const entry) const
{
   int32_t colour = SONGCOLOUR;

   //! \todo work out how to colour current song
   //if (nextSong->Id() == GetCurrentSong())
   //{
   //   colour = CURRENTSONGCOLOUR;
   //}
   //
   if ((search_.LastSearchString() != "") && (settings_.HightlightSearch() == true))
   {
      boost::regex expression(".*" + search_.LastSearchString() + ".*");

      if (((entry->type_ == ArtistType) && (boost::regex_match(entry->artist_, expression))) ||
          ((entry->type_ == AlbumType)  && (boost::regex_match(entry->album_,  expression))) ||
          ((entry->type_ == SongType)   && (boost::regex_match(entry->song_->Title(), expression))))
      {
         colour = SONGMATCHCOLOUR;
      } 
   }

   return colour;
}
