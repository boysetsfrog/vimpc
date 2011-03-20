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
#include "playlist.hpp"
#include "screen.hpp"
#include "search.hpp"
#include "settings.hpp"

#include <algorithm>
#include <boost/regex.hpp>
#include <iostream>

using namespace Ui;

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


void LibraryWindow::AddSong(Mpc::Song const * const song)
{
   static Mpc::LibraryEntry * LastArtistEntry = NULL;
   static Mpc::LibraryEntry * LastAlbumEntry  = NULL;
   static std::string    LastArtist      = "";
   static std::string    LastAlbum       = "";

   std::string artist = song->Artist();
   std::transform(artist.begin(), artist.end(), artist.begin(), ::toupper);

   std::string album = song->Album();
   std::transform(album.begin(), album.end(), album.begin(), ::toupper);
   
   if ((LastArtistEntry == NULL) || (LastArtist != artist))
   {
      Mpc::LibraryEntry * entry = NULL;

      LastAlbumEntry  = NULL;
      LastAlbum       = "";

      for (Mpc::Library::iterator it = buffer_.begin(); ((it != buffer_.end()) && (entry == NULL)); ++it)
      {
         std::string currentArtist = (*it)->artist_;
         std::transform(currentArtist.begin(), currentArtist.end(), currentArtist.begin(), ::toupper);

         if (currentArtist == artist)
         {
            entry           = (*it);
            LastArtistEntry = entry;
            LastArtist      = currentArtist;
         }
      }

      if (entry == NULL)
      {
         entry = new Mpc::LibraryEntry();

         entry->expanded_ = false;
         entry->artist_   = song->Artist();
         entry->type_     = Mpc::ArtistType;
         
         buffer_.push_back(entry);

         LastArtistEntry = entry;
         LastArtist      = artist;
      }
   }

   if ((LastAlbumEntry == NULL) || (LastAlbum != album))
   {
      Mpc::LibraryEntry * entry = NULL;

      for (Mpc::Library::iterator it = LastArtistEntry->children_.begin(); ((it != LastArtistEntry->children_.end()) && (entry == NULL)); ++it)
      {
         std::string currentAlbum = (*it)->album_;
         std::transform(currentAlbum.begin(), currentAlbum.end(), currentAlbum.begin(), ::toupper);

         if (currentAlbum == album)
         {
            entry           = (*it);
            LastAlbumEntry  = entry;
            LastAlbum       = currentAlbum;
         }
      }

      if (entry == NULL)
      {
         entry = new Mpc::LibraryEntry();

         entry->expanded_ = false;
         entry->artist_   = song->Artist();
         entry->album_    = song->Album();
         entry->type_     = Mpc::AlbumType;
         entry->parent_   = LastArtistEntry;

         LastArtistEntry->children_.push_back(entry);

         LastAlbumEntry = entry;
         LastAlbum      = album;
      }
   }

   if ((LastAlbumEntry != NULL) && (LastArtistEntry != NULL) && (LastArtist == artist) && (LastAlbum == album))
   {
      Mpc::LibraryEntry * const entry   = new Mpc::LibraryEntry();
      Mpc::Song * const    newSong = new Mpc::Song(*song);

      entry->expanded_ = true;
      entry->artist_   = song->Artist();
      entry->album_    = song->Album();
      entry->song_     = newSong;
      entry->type_     = Mpc::SongType;
      entry->parent_   = LastAlbumEntry;

      LastAlbumEntry->children_.push_back(entry);
   }
}


Mpc::Song * LibraryWindow::FindSong(Mpc::Song const * const song)
{
   std::string artist = song->Artist();
   std::transform(artist.begin(), artist.end(), artist.begin(), ::toupper);

   std::string album = song->Album();
   std::transform(album.begin(), album.end(), album.begin(), ::toupper);

   Mpc::LibraryEntry * artistEntry = NULL;
   Mpc::LibraryEntry * albumEntry  = NULL;
   
   for (Mpc::Library::iterator it = buffer_.begin(); ((it != buffer_.end()) && (artistEntry == NULL)); ++it)
   {
      std::string currentArtist = (*it)->artist_;
      std::transform(currentArtist.begin(), currentArtist.end(), currentArtist.begin(), ::toupper);

      if (currentArtist == artist)
      {
         artistEntry = (*it);
      }
   }

   if (artistEntry != NULL)
   {
      for (Mpc::Library::iterator it = artistEntry->children_.begin(); ((it != artistEntry->children_.end()) && (albumEntry == NULL)); ++it)
      {
         std::string currentAlbum = (*it)->album_;
         std::transform(currentAlbum.begin(), currentAlbum.end(), currentAlbum.begin(), ::toupper);

         if (currentAlbum == album)
         {
            albumEntry = (*it);
         }
      }
   }

   if (albumEntry != NULL)
   {
      for (Mpc::Library::iterator it = albumEntry->children_.begin(); (it != albumEntry->children_.end()); ++it)
      {
         if (*(*it)->song_ == *song)
         {
            return (*it)->song_;
         }
      }
   }

   if (albumEntry != NULL)
   {
      std::cout << "FOUND ALBUM" << std::endl;
   }

   std::cout << "huh?" << std::endl;

   return NULL;
}


void LibraryWindow::Redraw()
{
   Clear();
   client_.ForEachLibrarySong(*this, &LibraryWindow::AddSong);

   Mpc::LibraryEntry::LibraryEntryComparator Comparator;

   std::sort(buffer_.begin(), buffer_.end(), Comparator);

   for (Mpc::Library::iterator it = buffer_.begin(); it != buffer_.end(); ++it)
   {
      std::sort((*it)->children_.begin(), (*it)->children_.end(), Comparator);
   }
}

void LibraryWindow::Expand(uint32_t line)
{
   if ((buffer_.at(line)->expanded_ == false) && (buffer_.at(line)->type_ != Mpc::SongType))
   {
      buffer_.at(line)->expanded_ = true;

      Mpc::Library::iterator position = buffer_.begin();

      for (uint32_t i = 0; i <= line; ++i)
      {
         ++position;
      }

      for (Mpc::Library::reverse_iterator it = buffer_.at(line)->children_.rbegin(); it != buffer_.at(line)->children_.rend(); ++it)
      {
         position = buffer_.insert(position, *it);
      }
   }

   Scroll(1);
}

void LibraryWindow::Collapse(uint32_t line)
{
   if ((buffer_.at(line)->expanded_ == true) || (buffer_.at(line)->type_ != Mpc::ArtistType))
   {
      Mpc::LibraryEntry *     parent     = buffer_.at(line);
      Mpc::Library::iterator  position   = buffer_.begin();
      int                     parentLine = 0;

      if ((buffer_.at(line)->expanded_ == false) || (buffer_.at(line)->type_ == Mpc::SongType))
      {
         parent = buffer_.at(line)->parent_;
      }

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




std::string LibraryWindow::SearchPattern(UNUSED int32_t id)
{
   //! \todo add a search that searches in collapsed songs and
   //! expands things as necessary
   std::string pattern("");

   switch (buffer_.at(id)->type_)
   {
      case Mpc::ArtistType:
         pattern = buffer_.at(id)->artist_;
         break;
      
      case Mpc::AlbumType:
         pattern = buffer_.at(id)->album_;
         break;

      case Mpc::SongType:
         pattern = buffer_.at(id)->song_->Title();
         break;

      default:
         ASSERT(false);
         break;
   }

   return pattern;
}


void LibraryWindow::Clear()
{
   for (Mpc::Library::iterator it = buffer_.begin(); it != buffer_.end(); ++it)
   {
      delete *it;
   }

   buffer_.clear();
}

void LibraryWindow::Print(uint32_t line) const
{
   static std::string const BlankLine(screen_.MaxColumns(), ' ');

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

      if ((buffer_.at(printLine)->type_ == Mpc::AlbumType) || (buffer_.at(printLine)->type_ == Mpc::ArtistType))
      {
         uint8_t expandCol = 0;

         if (buffer_.at(printLine)->type_ == Mpc::AlbumType)
         {
            mvwprintw(window, line, 1, "|--");
            expandCol = 4;
         }
         
         wattron(window, A_BOLD); 
         mvwprintw(window, line, expandCol, "[ ]");

         if (printLine != CurrentLine()) 
         { 
            wattron(window, COLOR_PAIR(REDONDEFAULT)); 
         }
         
         char expand = (buffer_.at(printLine)->expanded_ == true) ? '-' : '+';
         mvwprintw(window, line, expandCol + 1, "%c", expand);

         if (printLine != CurrentLine()) 
         { 
            wattroff(window, COLOR_PAIR(REDONDEFAULT)); 
         }

         wattroff(window, A_BOLD);
         wattron(window, COLOR_PAIR(colour));
         wmove(window, line, expandCol + 4);

         if (buffer_.at(printLine)->type_ == Mpc::ArtistType)
         {
            waddstr(window, buffer_.at(printLine)->artist_.c_str());
         }
         else if (buffer_.at(printLine)->type_ == Mpc::AlbumType)
         {
            waddstr(window, buffer_.at(printLine)->album_.c_str());
         }

         wattroff(window, COLOR_PAIR(colour));
      }
      else if ((buffer_.at(printLine)->type_ == Mpc::SongType) && (buffer_.at(printLine)->song_ != NULL))
      {
         mvwprintw(window, line, 1, "|   |--");

         wattron(window, A_BOLD);
         wprintw(window, "[");

         if (printLine != CurrentLine()) 
         {
            wattron(window, COLOR_PAIR(REDONDEFAULT)); 
         }

         wprintw(window, "%2s" , buffer_.at(printLine)->song_->Track().c_str());

         if (printLine != CurrentLine()) 
         {
            wattroff(window, COLOR_PAIR(REDONDEFAULT)); 
         }
         
         wprintw(window, "]");

         if (colour != CURRENTSONGCOLOUR)
         {
            wattroff(window, A_BOLD);
         }

         waddstr(window, " ");
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
}

void LibraryWindow::Confirm()
{
   client_.Clear();
   
   AddSongsToPlaylist(Mpc::Song::Single);

   client_.Play(0);
   screen_.Redraw(Ui::Screen::Playlist);
}

void LibraryWindow::AddSongsToPlaylist(Mpc::Song::SongCollection Collection)
{
   if (Collection == Mpc::Song::Single)
   {
      AddSongsToPlaylist(buffer_.at(CurrentLine()));
   }
   else
   {
      for (Mpc::Library::iterator it = buffer_.begin(); it != buffer_.end(); ++it)
      {
         if ((*it)->type_ == Mpc::ArtistType)
         {
            AddSongsToPlaylist((*it));
         }
      }
   }
}

void LibraryWindow::AddSongsToPlaylist(Mpc::LibraryEntry const * const entry)
{
   if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL))
   {
      Mpc::Playlist::Instance().Add(entry->song_);
      client_.Add(*(entry->song_));
   }
   else 
   {
      for (Mpc::Library::const_iterator it = entry->children_.begin(); it != entry->children_.end(); ++it)
      {
         AddSongsToPlaylist((*it));
      }
   }
}

// \todo should store the current state for each album artist entry in the library
// and update it when there is an add or delete that way i won't need to do a recursive loop
// here for every single print, which is plenty slow
int32_t LibraryWindow::DetermineSongColour(Mpc::LibraryEntry const * const entry) const
{
   int32_t colour = SONGCOLOUR;

   if ((search_.LastSearchString() != "") && (settings_.HightlightSearch() == true))
   {
      boost::regex expression(".*" + search_.LastSearchString() + ".*");

      if (((entry->type_ == Mpc::ArtistType) && (boost::regex_match(entry->artist_, expression))) ||
          ((entry->type_ == Mpc::AlbumType)  && (boost::regex_match(entry->album_,  expression))) ||
          ((entry->type_ == Mpc::SongType)   && (boost::regex_match(entry->song_->Title(), expression))))
      {
         colour = SONGMATCHCOLOUR;
      } 
   }

   //! \todo this needs to be dramatically improved in speed it really is a PoC at the moment
   //        and is way to slow to be usable in anyway
   if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL) && (client_.SongIsInQueue(*entry->song_) == true))
   {
      colour = GREENONDEFAULT;
   }
   else if ((entry->type_ != Mpc::SongType) && (entry->children_.size() > 0))
   {
      Mpc::Library::const_iterator it = entry->children_.begin();

      unsigned int count = 0;

      for (; (it != entry->children_.end()); ++it)
      {
         int32_t newColour = DetermineSongColour(*it);

         if ((newColour == GREENONDEFAULT) || (newColour == CURRENTSONGCOLOUR) || (newColour == CYANONDEFAULT))
         {
            if ((newColour == GREENONDEFAULT) || (newColour == CURRENTSONGCOLOUR))
            {
               count++;
            }
            colour = CYANONDEFAULT;
         }
      }

      if (count == entry->children_.size())
      {
         colour = GREENONDEFAULT;
      }
   }
   
   if ((entry->song_ != NULL) && (entry->song_->URI() == client_.GetCurrentSongURI()))
   {
      colour = CURRENTSONGCOLOUR;
   }

   return colour;
}
