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

const std::string VariousArtist = "Various Artists";

using namespace Mpc;

Library::Library() :
   settings_     (Main::Settings::Instance()),
   variousArtist_(NULL)
{
   AddCallback(Main::Buffer_Remove, new CallbackObject(*this, &Library::CheckIfVariousRemoved));
}

Library::~Library()
{
   Clear();
}

void Library::Clear(bool Delete)
{
   Main::Playlist().Clear();

   while (Size() > 0)
   {
      int const Pos = Size() - 1;
      LibraryEntry * entry = Get(Pos);
      Remove(Pos, 1);

      if ((Delete == true) && (entry->parent_ == NULL))
      {
         delete entry;
      }
   }
}

void Library::Add(Mpc::Song * song)
{
   static Mpc::LibraryEntry * LastArtistEntry    = NULL;
   static Mpc::LibraryEntry * LastAlbumEntry     = NULL;
   static std::string         LastArtist         = "";
   static std::string         LastAlbum          = "";

   std::string artist = song->Artist();
   std::string album  = song->Album();

   if ((LastArtistEntry == NULL) ||
       (((Algorithm::iequals(LastArtist, artist) == false)) &&
         ((LastAlbumEntry == NULL) || (Algorithm::iequals(LastAlbum, album) == false))))
   {
      Mpc::LibraryEntry * entry = NULL;

      LastAlbumEntry  = NULL;
      LastAlbum       = "";

      uint32_t i = 0;

      for (i = 0; ((i < Size()) && 
                   ((Algorithm::iequals(Get(i)->artist_, artist) == false) ||
                   (Get(i)->type_ != Mpc::ArtistType))); ++i);

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
         if ((Algorithm::iequals((*it)->album_, album) == true) &&
             ((*it)->type_ == Mpc::AlbumType))
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

   if ((LastAlbumEntry != NULL) && (LastArtistEntry != NULL) &&
       //(Algorithm::iequals(LastArtist, artist) == true) &&
       (Algorithm::iequals(LastAlbum, album) == true))
   {
      Mpc::LibraryEntry * const entry   = new Mpc::LibraryEntry();

      if ((Algorithm::iequals(LastArtist, artist) == false) &&
          (LastArtist != VariousArtist))
      {
         LastArtist = VariousArtist;

         if (variousArtist_ == NULL)
         {
            variousArtist_ = new Mpc::LibraryEntry();
            variousArtist_->expanded_ = false;
            variousArtist_->artist_   = VariousArtist;
            variousArtist_->type_     = Mpc::ArtistType;
            Add(variousArtist_);
         }

         LastArtistEntry->children_.pop_back();

         if (LastArtistEntry->children_.size() == 0)
         {
            Remove(Index(LastArtistEntry), 1);
            delete LastArtistEntry;
         }

         variousArtist_->children_.push_back(LastAlbumEntry);
         LastAlbumEntry->parent_ = variousArtist_;
         LastArtistEntry = variousArtist_;
      }

      entry->expanded_ = true;
      entry->artist_   = artist;
      entry->album_    = album;
      entry->song_     = song;
      entry->type_     = Mpc::SongType;
      entry->parent_   = LastAlbumEntry;

      song->SetEntry(entry);

      uriMap_[song->URI()] = song;

      LastAlbumEntry->children_.push_back(entry);
   }
}

Mpc::Song * Library::Song(std::string uri) const
{
   std::map<std::string, Mpc::Song *>::const_iterator it = uriMap_.find(uri);

   if (it != uriMap_.end())
   {
      return it->second;
   }

   return NULL;
}


void Library::Sort()
{
   Mpc::LibraryEntry::LibraryComparator comparator;

   Main::Buffer<Library::BufferType>::Sort(comparator);

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
   if (position < Size())
   {
      Mpc::CommandList list(client);

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
}

void Library::RemoveFromPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, uint32_t position)
{
   if (position < Size())
   {
      Mpc::CommandList list(client);

      if (Collection == Mpc::Song::Single)
      {
         RemoveFromPlaylist(client, Get(position));
      }
      else
      {
         for (uint32_t i = 0; i < Size(); ++i)
         {
            RemoveFromPlaylist(client, Get(i));
         }
      }
   }
}

void Library::AddToPlaylist(Mpc::Client & client, Mpc::LibraryEntry const * const entry, int32_t position)
{
   if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL))
   {
      if (position != -1)
      {
         Main::Playlist().Add(entry->song_, position);
         client.Add(*(entry->song_), position);
      }
      else if ((Main::Settings::Instance().Get(Setting::AddPosition) == Setting::AddEnd) ||
          (client.GetCurrentSong() == -1))
      {
         Main::Playlist().Add(entry->song_);
         client.Add(*(entry->song_));
      }
      else
      {
         Main::Playlist().Add(entry->song_, client.GetCurrentSong() + 1);
         client.Add(*(entry->song_), client.GetCurrentSong() + 1);
      }
   }
   else
   {
      int current = -1;

      if ((Main::Settings::Instance().Get(Setting::AddPosition) == Setting::AddNext) &&
          (client.GetCurrentSong() != -1))
      {
         current = client.GetCurrentSong() + 1;
      }

      for (Mpc::LibraryEntryVector::const_iterator it = entry->children_.begin(); it != entry->children_.end(); ++it)
      {
         AddToPlaylist(client, (*it), current);

         if (current != -1)
         {
            current++;
         }
      }
   }
}

void Library::RemoveFromPlaylist(Mpc::Client & client, Mpc::LibraryEntry const * const entry)
{
   if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL))
   {
      int32_t PlaylistIndex = Main::Playlist().Index(entry->song_);

      if (PlaylistIndex >= 0)
      {
         client.Delete(PlaylistIndex);
         Main::Playlist().Remove(PlaylistIndex, 1);
      }
   }
   else
   {
      for (Mpc::LibraryEntryVector::const_iterator it = entry->children_.begin(); it != entry->children_.end(); ++it)
      {
         RemoveFromPlaylist(client, (*it));
      }
   }
}

void Library::ForEachChild(uint32_t index, Main::CallbackInterface<Mpc::Song *> * callback) const
{
   for (Mpc::LibraryEntryVector::iterator it = Get(index)->children_.begin(); (it != Get(index)->children_.end()); ++it)
   {
      if ((*it)->type_ == AlbumType)
      {
         for (Mpc::LibraryEntryVector::iterator jt = (*it)->children_.begin(); (jt != (*it)->children_.end()); ++jt)
         {
            (*callback)((*jt)->song_);
         }
      }
      else if ((*it)->type_ == SongType)
      {
         (*callback)((*it)->song_);
      }
   }
}

void Library::ForEachChild(uint32_t index, Main::CallbackInterface<Mpc::LibraryEntry *> * callback) const
{
   for (Mpc::LibraryEntryVector::iterator it = Get(index)->children_.begin(); (it != Get(index)->children_.end()); ++it)
   {
      if ((*it)->type_ == AlbumType)
      {
         for (Mpc::LibraryEntryVector::iterator jt = (*it)->children_.begin(); (jt != (*it)->children_.end()); ++jt)
         {
            (*callback)(*jt);
         }
      }

      (*callback)(*it);
   }
}

void Library::ForEachSong(Main::CallbackInterface<Mpc::Song *> * callback) const
{
   for (uint32_t i = 0; i < Size(); ++i)
   {
      if (Get(i)->type_ == ArtistType)
      {
         for (Mpc::LibraryEntryVector::iterator it = Get(i)->children_.begin(); (it != Get(i)->children_.end()); ++it)
         {
            if ((*it)->type_ == AlbumType)
            {
               for (Mpc::LibraryEntryVector::iterator jt = (*it)->children_.begin(); (jt != (*it)->children_.end()); ++jt)
               {
                  (*callback)((*jt)->song_);
               }
            }
         }
      }
   }
}

void Library::ForEachParent(Main::CallbackInterface<Mpc::LibraryEntry *> * callback) const
{
   for (uint32_t i = 0; i < Size(); ++i)
   {
      if (Get(i)->type_ == ArtistType)
      {
         for (Mpc::LibraryEntryVector::iterator it = Get(i)->children_.begin(); (it != Get(i)->children_.end()); ++it)
         {
            if ((*it)->type_ == AlbumType)
            {
               (*callback)(*it);
            }
         }

         (*callback)(Get(i));
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
   Mpc::LibraryEntry * const entry     = Get(line);
   Mpc::LibraryEntry * const parent    = entry->parent_;
   Mpc::LibraryEntry * entryToCollapse = entry;

   if ((entry->expanded_ == false) || (entry->type_ == Mpc::SongType))
   {
      entryToCollapse = parent;
   }

   if (Index(entryToCollapse) >= 0)
   {
      uint32_t Pos = Index(entryToCollapse);
      Main::CallbackInterface<LibraryEntry *> * callback = new CallbackObject(*this, &Library::RemoveAndUnexpand);
      ForEachChild(Pos, callback);
      delete callback;

      entryToCollapse->expanded_ = false;
   }
}


std::string Library::String(uint32_t position) const
{
   if (Get(position)->type_ == Mpc::ArtistType)
   {
      return Get(position)->artist_;
   }
   else if (Get(position)->type_ == Mpc::AlbumType)
   {
      return Get(position)->album_;
   }
}

std::string Library::PrintString(uint32_t position) const
{
   if (Get(position)->type_ == Mpc::ArtistType)
   {
      return " $B" + Get(position)->artist_ + "$B";
   }
   else if (Get(position)->type_ == Mpc::AlbumType)
   {
      return "    " + Get(position)->album_;
   }
   else if (Get(position)->type_ == Mpc::SongType)
   {
      return "       " + Get(position)->song_->FormatString(settings_.Get(Setting::LibraryFormat));
   }
}


void Library::RemoveAndUnexpand(LibraryEntry * const entry)
{
   if (Index(entry) != -1)
   {
      Remove(Index(entry), 1);
      entry->expanded_ = false;
   }
}


void Library::CheckIfVariousRemoved(LibraryEntry * const entry)
{
   if (entry == variousArtist_)
   {
      variousArtist_ = NULL;
   }
}


void Mpc::MarkUnexpanded(LibraryEntry * const entry)
{
   entry->expanded_ = false;
}

/* vim: set sw=3 ts=3: */
