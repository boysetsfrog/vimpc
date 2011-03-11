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

//! \todo the tracks are not sorted properly within the albums

LibraryWindow::LibraryWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (screen),
   settings_        (settings),
   client_          (client),
   search_          (search),
   buffer_          ()
{
}

LibraryWindow::~LibraryWindow()
{
   Clear();
}


void LibraryWindow::AddSong(UNUSED Mpc::Song const * const song)
{
   static LibraryEntry * LastArtistEntry = NULL;
   static LibraryEntry * LastAlbumEntry  = NULL;
   static std::string    LastArtist      = "";
   static std::string    LastAlbum       = "";

   std::string artist = song->Artist();
   std::transform(artist.begin(), artist.end(), artist.begin(), ::toupper);

   std::string album = song->Album();
   std::transform(album.begin(), album.end(), album.begin(), ::toupper);
   
   if ((LastArtistEntry == NULL) || (LastArtist != artist))
   {
      LibraryEntry * const entry = new LibraryEntry();

      entry->expanded_ = false;
      entry->artist_   = song->Artist();
      entry->type_     = ArtistType;
      
      buffer_.push_back(entry);

      LastArtistEntry = entry;
      LastArtist      = artist;
   }

   if ((LastAlbumEntry == NULL) || (LastAlbum != album))
   {
      LibraryEntry * const entry = new LibraryEntry();

      entry->expanded_ = false;
      entry->artist_   = song->Artist();
      entry->album_    = song->Album();
      entry->type_     = AlbumType;
      entry->parent_   = LastArtistEntry;

      LastArtistEntry->children_.push_back(entry);

      LastAlbumEntry = entry;
      LastAlbum      = album;
   }

   if ((LastAlbumEntry != NULL) && (LastArtistEntry != NULL) && (LastArtist == artist) && (LastAlbum == album))
   {
      LibraryEntry * const entry   = new LibraryEntry();
      Mpc::Song * const    newSong = new Mpc::Song(*song);

      entry->expanded_ = true;
      entry->artist_   = song->Artist();
      entry->album_    = song->Album();
      entry->song_     = newSong;
      entry->type_     = SongType;
      entry->parent_   = LastAlbumEntry;

      LastAlbumEntry->children_.push_back(entry);
   }
}


void LibraryWindow::Redraw()
{
   Clear();
   client_.ForEachLibrarySong(*this, &LibraryWindow::AddSong);
}


std::string LibraryWindow::SearchPattern(UNUSED int32_t id)
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


void LibraryWindow::Expand(UNUSED uint32_t line)
{
   if (buffer_.at(line)->expanded_ == false)
   {
      buffer_.at(line)->expanded_ = true;

      Library::iterator position = buffer_.begin();

      for (uint32_t i = 0; i <= line; ++i)
      {
         ++position;
      }

      for (Library::reverse_iterator it = buffer_.at(line)->children_.rbegin(); it != buffer_.at(line)->children_.rend(); ++it)
      {
         position = buffer_.insert(position, *it);
      }
   }
}

void LibraryWindow::Collapse(UNUSED uint32_t line)
{
   if ((buffer_.at(line)->expanded_ == true) || (buffer_.at(line)->type_ != ArtistType))
   {
      LibraryEntry * const parent     = buffer_.at(line)->parent_;
      Library::iterator    position   = buffer_.begin();
      int                  parentLine = 1;

      parent->expanded_ = false;

      for(; (((*position) != parent) && (position != buffer_.end())); ++position, ++parentLine);

      if (position != buffer_.end())
      {
         ++position;

         for (; position != buffer_.end() && (((*position)->parent_ == parent) || (((*position)->parent_ != NULL) && ((*position)->parent_->parent_ == parent)));)
         {
            (*position)->expanded_ = false;
            position = buffer_.erase(position);
         }
      }

      ScrollTo(parentLine);
   }
   else
   {
      Scroll(-1);
   }
}

void LibraryWindow::Clear()
{
   for (Library::iterator it = buffer_.begin(); it != buffer_.end(); ++it)
   {
      delete *it;
   }

   buffer_.clear();
}

void LibraryWindow::Print(uint32_t line) const
{
   //! \todo make songs that are currently in the playlist display a different colour?
   static std::string const BlankLine(screen_.MaxColumns(), ' ');
   static int32_t trackCount = 0;

   WINDOW * window = N_WINDOW();

   if ((line + FirstLine()) < BufferSize())
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

         if (colour != CURRENTSONGCOLOUR)
         {
            wattroff(window, A_BOLD);
         }

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

      wattroff(window, A_BOLD | A_REVERSE | COLOR_PAIR(colour));
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
   
   int32_t song = AddSongsToPlaylist(Mpc::Song::Single);

   if (song != -1)
   {
      client_.Play(song);
   }
}

int32_t LibraryWindow::AddSongsToPlaylist(Mpc::Song::SongCollection Collection)
{
   int32_t songToPlay = -1;

   if (Collection == Mpc::Song::Single)
   {
      AddSongsToPlaylist(buffer_.at(CurrentLine()), songToPlay);
   }
   else
   {
      for (Library::iterator it = buffer_.begin(); it != buffer_.end(); ++it)
      {
         if ((*it)->type_ == ArtistType)
         {
            AddSongsToPlaylist((*it), songToPlay);
         }
      }
   }

   return songToPlay;
}

void LibraryWindow::AddSongsToPlaylist(LibraryEntry const * const entry, int32_t & songToPlay)
{
   if ((entry->type_ == SongType) && (entry->song_ != NULL))
   {
       int32_t const songAdded = client_.Add(*(entry->song_));

       if (songToPlay == -1)
       {
          songToPlay = songAdded;
       }
   }
   else 
   {
      for (Library::const_iterator it = entry->children_.begin(); it != entry->children_.end(); ++it)
      {
         AddSongsToPlaylist((*it), songToPlay);
      }
   }
}

int32_t LibraryWindow::DetermineSongColour(LibraryEntry const * const entry) const
{
   int32_t colour = SONGCOLOUR;

   //! \todo work out how to colour current song
   if ((entry->song_ != NULL) && (entry->song_->URI() == client_.GetCurrentSongURI()))
   {
      colour = CURRENTSONGCOLOUR;
   }
   
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
