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

#include "algorithm.hpp"
#include "mpdclient.hpp"
#include "playlist.hpp"

#include <algorithm>

using namespace Mpc;

Library::Library()
{
   AddCallback(Main::Buffer_Remove, new CallbackFunction(&Mpc::MarkUnexpanded));
}

Library::~Library()
{
}

void Library::Add(UNUSED Mpc::Song const * const song)
{
   static Mpc::LibraryEntry * LastArtistEntry = NULL;
   static Mpc::LibraryEntry * LastAlbumEntry  = NULL;
   static std::string         LastArtist      = "";
   static std::string         LastAlbum       = "";

   std::string artist = song->Artist();
   std::string album  = song->Album();
   
   if ((LastArtistEntry == NULL) || (Algorithm::iequals(LastArtist, artist) == false))
   {
      Mpc::LibraryEntry * entry = NULL;

      LastAlbumEntry  = NULL;
      LastAlbum       = "";

      uint32_t i = 0;

      for (i = 0; ((i < Size()) && (Algorithm::iequals(Get(i)->artist_, artist) == false)); ++i);

      if (i < Size())
      {
         entry = Get(i);
      }

      if (entry == NULL)
      {
         entry            = new Mpc::LibraryEntry();
         entry->expanded_ = false;
         entry->artist_   = artist;
         entry->type_     = Mpc::ArtistType;
         
         Add(entry);
      }

      LastArtistEntry = entry;
      LastArtist      = artist;
   }

   if ((LastAlbumEntry == NULL) || (Algorithm::iequals(LastAlbum, album) == false))
   {
      Mpc::LibraryEntry * entry = NULL;

      for (Mpc::LibraryEntryVector::iterator it = LastArtistEntry->children_.begin(); ((it != LastArtistEntry->children_.end()) && (entry == NULL)); ++it)
      {

         if (Algorithm::iequals((*it)->album_, album) == true)
         {
            entry           = (*it);
            LastAlbumEntry  = entry;
            LastAlbum       = album;
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

   if ((LastAlbumEntry != NULL) && (LastArtistEntry != NULL) && (Algorithm::iequals(LastArtist, artist) == true) && (Algorithm::iequals(LastAlbum, album) == true))
   {
      Mpc::LibraryEntry * const entry   = new Mpc::LibraryEntry();
      Mpc::Song * const         newSong = new Mpc::Song(*song);

      entry->expanded_ = true;
      entry->artist_   = artist;
      entry->album_    = album;
      entry->song_     = newSong;
      entry->type_     = Mpc::SongType;
      entry->parent_   = LastAlbumEntry;

      LastAlbumEntry->children_.push_back(entry);
   }
}

Mpc::Song * Library::Song(Mpc::Song const * const song)
{
   std::string artist = song->Artist();
   std::string album  = song->Album();

   Mpc::LibraryEntry * artistEntry = NULL;
   Mpc::LibraryEntry * albumEntry  = NULL;
   
   for (uint32_t i = 0; ((i <= Size()) && (artistEntry == NULL)); ++i)
   {
      if (Algorithm::iequals((Get(i))->artist_, artist) == true)
      {
         artistEntry = Get(i);
      }
   }

   if (artistEntry != NULL)
   {
      for (Mpc::LibraryEntryVector::iterator it = artistEntry->children_.begin(); ((it != artistEntry->children_.end()) && (albumEntry == NULL)); ++it)
      {
         if (Algorithm::iequals((*it)->album_ ,album) == true)
         {
            albumEntry = (*it);
         }
      }

      if (albumEntry != NULL)
      {
         for (Mpc::LibraryEntryVector::iterator it = albumEntry->children_.begin(); (it != albumEntry->children_.end()); ++it)
         {
            if (*(*it)->song_ == *song)
            {
               return (*it)->song_;
            }
         }
      }
   }

   return NULL;
}

void Library::Sort()
{
   Mpc::LibraryEntry::LibraryComparator UNUSED entryComparator;
   Main::Buffer<Library::BufferType>::Sort(entryComparator);

   for (uint32_t i = 0; (i < Size()); ++i)
   {
      Sort(Get(i));
   }
}

void Library::Sort(LibraryEntry * entry)
{
   Mpc::LibraryEntry::LibraryComparator entryComparator;

   if (entry->children_.empty() == false)
   {
       std::sort(entry->children_.begin(), entry->children_.end(), entryComparator);

       for (LibraryEntryVector::iterator it = entry->children_.begin(); it != entry->children_.end(); ++it)
       {
          if ((*it)->children_.empty() == false)
          {
             Sort(*it);
          }
       }
   }
}

void Library::AddToPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, uint32_t position)
{
   if (Collection == Mpc::Song::Single)
   {
      AddToPlaylist(client, Get(position));
   }
   else
   {
      for (uint32_t i = 0; i < Size(); ++i)
      {
         AddToPlaylist(client, Get(i));
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
   else if (entry->expanded_ == false)
   {
      for (Mpc::LibraryEntryVector::const_iterator it = entry->children_.begin(); it != entry->children_.end(); ++it)
      {
         AddToPlaylist(client, (*it));
      }
   }
}


void Library::Expand(uint32_t line)
{
   uint32_t position = line;

   if ((Get(line)->expanded_ == false) && (Get(line)->type_ != Mpc::SongType))
   {
      Get(line)->expanded_ = true;

      for (Mpc::LibraryEntryVector::iterator it = Get(line)->children_.begin(); it != Get(line)->children_.end(); ++it)
      {
         Add(*it, ++position);
      }
   }
}

void Library::Collapse(uint32_t line)
{
   //! \todo needs cleaning up in terms of name and making much more efficient
   //! really it's just plain ugly
   Mpc::LibraryEntry * const entry     = Get(line);
   Mpc::LibraryEntry * const parent    = entry->parent_;
   Mpc::LibraryEntry * entryToCollapse = entry;

   if ((entry->expanded_ == false) || (entry->type_ == Mpc::SongType))
   {
      entryToCollapse = parent;
   }

   if ((entryToCollapse != NULL) && (entryToCollapse->expanded_ == true))
   {
      uint32_t last = 0;
      
      if ((entryToCollapse->children_.size() != 0) && (entryToCollapse->children_.back()->expanded_ == true))
      {
         last += entryToCollapse->children_.back()->children_.size();
      }

      Remove(Index(entryToCollapse) + 1, Index(entryToCollapse->children_.back()) + last - Index(entryToCollapse));
      entryToCollapse->expanded_ = false;
   }
}


void Mpc::MarkUnexpanded(LibraryEntry * const entry)
{
   entry->expanded_ = false;
}
