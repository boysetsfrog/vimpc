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

#include "mpdclient.hpp"
#include "playlist.hpp"

#include <algorithm>

using namespace Mpc;

void Library::Clear()
{
   for (iterator it = begin(); it != end(); ++it)
   {
      delete *it;
   }
}

void Library::Add(Mpc::Song const * const song)
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

      for (iterator it = begin(); ((it != end()) && (entry == NULL)); ++it)
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
         
         push_back(entry);

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

Mpc::Song * Library::Song(Mpc::Song const * const song)
{
   std::string artist = song->Artist();
   std::transform(artist.begin(), artist.end(), artist.begin(), ::toupper);

   std::string album = song->Album();
   std::transform(album.begin(), album.end(), album.begin(), ::toupper);

   Mpc::LibraryEntry * artistEntry = NULL;
   Mpc::LibraryEntry * albumEntry  = NULL;
   
   for (iterator it = begin(); ((it != end()) && (artistEntry == NULL)); ++it)
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

   return NULL;
}

void Library::Sort()
{
   Mpc::LibraryEntry::LibraryEntryComparator Comparator;

   std::sort(begin(), end(), Comparator);

   for (iterator it = begin(); it != end(); ++it)
   {
      std::sort((*it)->children_.begin(), (*it)->children_.end(), Comparator);
   }
}

void Library::AddToPlaylist(Mpc::Client & client, Mpc::Song::SongCollection Collection, uint32_t position)
{
   if (Collection == Mpc::Song::Single)
   {
      AddToPlaylist(client, at(position));
   }
   else
   {
      for (iterator it = begin(); it != end(); ++it)
      {
         if ((*it)->type_ == Mpc::ArtistType)
         {
            AddToPlaylist(client, (*it));
         }
      }
   }
}

void Library::AddToPlaylist(Mpc::Client & client, Mpc::LibraryEntry const * const entry)
{
   if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL))
   {
      Mpc::Playlist::Instance().Add(entry->song_);
      client.Add(*(entry->song_));
   }
   else 
   {
      for (Mpc::LibraryEntryVector::const_iterator it = entry->children_.begin(); it != entry->children_.end(); ++it)
      {
         AddToPlaylist(client, (*it));
      }
   }
}


void Library::Expand(uint32_t line)
{
   if ((Entry(line)->expanded_ == false) && (Entry(line)->type_ != Mpc::SongType))
   {
      Entry(line)->expanded_ = true;

      Mpc::Library::iterator position = begin();

      for (uint32_t i = 0; i <= line; ++i)
      {
         ++position;
      }

      for (Mpc::LibraryEntryVector::reverse_iterator it = Entry(line)->children_.rbegin(); it != Entry(line)->children_.rend(); ++it)
      {
         position = insert(position, *it);
      }
   }
}

void Library::Collapse(uint32_t line)
{
   if ((Entry(line)->expanded_ == true) || (Entry(line)->type_ != Mpc::ArtistType))
   {
      Mpc::LibraryEntry *     parent     = Entry(line);
      Mpc::Library::iterator  position   = begin();

      if ((Entry(line)->expanded_ == false) || (Entry(line)->type_ == Mpc::SongType))
      {
         parent = Entry(line)->parent_;
      }

      parent->expanded_ = false;

      for(; (((*position) != parent) && (position != end())); ++position);

      if (position != end())
      {
         ++position;

         for (; position != end() && (((*position)->parent_ == parent) || (((*position)->parent_ != NULL) && ((*position)->parent_->parent_ == parent)));)
         {
            (*position)->expanded_ = false;
            position = erase(position);
         }
      }
   }
}
