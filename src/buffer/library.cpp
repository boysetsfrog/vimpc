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
#include "browse.hpp"
#include "clientstate.hpp"
#include "directory.hpp"
#include "events.hpp"
#include "mpdclient.hpp"
#include "playlist.hpp"
#include "vimpc.hpp"

#include <algorithm>

const std::string VariousArtist = "Various Artists";

using namespace Mpc;

Library::Library() :
   settings_       (Main::Settings::Instance()),
   variousArtist_  (NULL),
   lastAlbumEntry_ (NULL),
   lastArtistEntry_(NULL)
{
   AddCallback(Main::Buffer_Remove, [this] (LibraryEntry * const entry) { CheckIfVariousRemoved(entry); });

   Main::Vimpc::EventHandler(Event::AllMetaDataReady, [this] (EventData const & Data) { Sort(); });

   settings_.RegisterCallback(Setting::AlbumArtist, [this] (bool Value)
   {
      // Clear the cached format prints for every song
      for (auto song : uriMap_)
      {
         song.second->FormatString("");
      }

      RecreateLibraryFromURIs();
   });
}

Library::~Library()
{
   Clear();
}

void Library::Clear(bool Delete)
{
   lastAlbumEntry_   = NULL;
   lastArtistEntry_  = NULL;

   Main::Playlist().Clear();

   // \todo investigate this, adds considerable overhead in callgrind
   Main::Browse().Clear();

   uriMap_.clear();

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

void Library::RecreateLibraryFromURIs()
{
   // collapse everything
   ForEachParent([] (Mpc::LibraryEntry * entry) { Mpc::MarkUnexpanded(entry); });

   // clear the songs
   FUNCTION<void (LibraryEntry *)> function = [this] (LibraryEntry * entry) { entry->song_ = NULL; };

   for (int i = 0; i < Size(); ++i)
   {
      ForEachChild(i, function);
   }

   // get rid of every entry
   while (Size() > 0)
   {
      int const Pos = Size() - 1;
      LibraryEntry * entry = Get(Pos);
      Remove(Pos, 1);

      if (entry->parent_ == NULL)
      {
         delete entry;
      }
   }

   for (auto song : uriMap_)
   {
      Add(song.second);
   }
}

void Library::Add(Mpc::Song * song)
{
   std::string artist = song->Artist();
   std::string const album  = song->Album();
   std::string const albumartist = song->AlbumArtist();

   if ((settings_.Get(Setting::AlbumArtist) == true) && (albumartist != "Unknown Artist"))
   {
      artist = albumartist;
   }

   CreateVariousArtist();

   if ((lastAlbumEntry_ == NULL) ||
       (Algorithm::iequals(lastAlbumEntry_->album_, album,
                          settings_.Get(Setting::IgnoreTheGroup), true) == false))
   {
      lastAlbumEntry_  = NULL;

      if ((lastArtistEntry_ == NULL) ||
          (Algorithm::iequals(lastArtistEntry_->artist_, artist) == false))
      {
         lastArtistEntry_ = NULL;

         uint32_t i = 0;

         for (i = 0;
              ((i < Size()) &&
               ((Get(i)->type_ != Mpc::ArtistType) ||
                (Algorithm::iequals(Get(i)->artist_, artist,
                                   settings_.Get(Setting::IgnoreTheGroup), true) == false)));
               ++i);

         if (i < Size())
         {
            lastArtistEntry_ = Get(i);
         }
         else
         {
            lastArtistEntry_ = CreateArtistEntry(artist);
         }
      }

      for (auto entry : lastArtistEntry_->children_)
      {
         if ((entry->type_ == Mpc::AlbumType) &&
             (Algorithm::iequals(entry->album_, album) == true))
         {
            lastAlbumEntry_ = entry;
            break;
         }
      }

      if (lastAlbumEntry_ == NULL)
      {
         lastAlbumEntry_ = CreateAlbumEntry(song);
         lastAlbumEntry_->parent_ = lastArtistEntry_;
         lastArtistEntry_->children_.push_back(lastAlbumEntry_);
      }
   }

   if ((lastArtistEntry_ != NULL) && (lastAlbumEntry_ != NULL) &&
       (lastArtistEntry_ != variousArtist_) &&
       (lastArtistEntry_->children_.back() == lastAlbumEntry_) &&
       (Algorithm::iequals(lastAlbumEntry_->album_, album)  == true) &&
       (Algorithm::iequals(lastArtistEntry_->artist_, artist,
           settings_.Get(Setting::IgnoreTheGroup), true) == false))
   {
      lastArtistEntry_->children_.pop_back();

      if (lastArtistEntry_->children_.size() == 0)
      {
         Remove(Index(lastArtistEntry_), 1);
         delete lastArtistEntry_;
      }

      if (lastAlbumEntry_->parent_ != variousArtist_)
      {
         variousArtist_->children_.push_back(lastAlbumEntry_);
      }

      lastAlbumEntry_->parent_ = variousArtist_;
      lastArtistEntry_ = variousArtist_;
   }

   Mpc::LibraryEntry * const entry = new Mpc::LibraryEntry();
   entry->expanded_ = true;
   entry->artist_   = artist;
   entry->album_    = album;
   entry->song_     = song;
   entry->type_     = Mpc::SongType;
   entry->parent_   = lastAlbumEntry_;
   song->SetEntry(entry);

   uriMap_[song->URI()] = song;

   if (lastAlbumEntry_ != NULL)
   {
      lastAlbumEntry_->children_.push_back(entry);
   }
}

void Library::CreateVariousArtist()
{
   if (variousArtist_ == NULL)
   {
      variousArtist_ = new Mpc::LibraryEntry();
      variousArtist_->expanded_ = false;
      variousArtist_->artist_   = VariousArtist;
      variousArtist_->type_     = Mpc::ArtistType;
      Add(variousArtist_);
   }
}

Mpc::LibraryEntry * Library::CreateArtistEntry(std::string artist)
{
   Mpc::LibraryEntry * const entry = new Mpc::LibraryEntry();

   entry->expanded_ = false;
   entry->artist_   = artist;
   entry->type_     = Mpc::ArtistType;

   Add(entry);
   return entry;
}

Mpc::LibraryEntry * Library::CreateAlbumEntry(Mpc::Song * song)
{
   Mpc::LibraryEntry * const entry = new Mpc::LibraryEntry();
   entry->expanded_ = false;
   entry->artist_   = song->Artist();
   entry->album_    = song->Album();
   entry->type_     = Mpc::AlbumType;
   return entry;
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

       for (auto child : entry->children_)
       {
          if (child->children_.empty() == false)
          {
             Sort(child);
          }
       }
   }
}

void Library::AddToPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, Mpc::ClientState & clientState, uint32_t position)
{
   if (position < Size())
   {
      if (Collection == Mpc::Song::Single)
      {
         AddToPlaylist(client, clientState, Get(position));
      }
      else
      {
         Mpc::CommandList list(client);

         for (uint32_t i = 0; i < Size(); ++i)
         {
            AddToPlaylist(client, clientState, Get(i));
         }
      }
   }
}

void Library::RemoveFromPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, Mpc::ClientState & clientState, uint32_t position)
{
   if (position < Size())
   {
      if (Collection == Mpc::Song::Single)
      {
         RemoveFromPlaylist(client, Get(position));
      }
      else
      {
         Mpc::CommandList list(client);

         for (uint32_t i = 0; i < Size(); ++i)
         {
            RemoveFromPlaylist(client, Get(i));
         }
      }
   }
}

void Library::AddToPlaylist(Mpc::Client & client, Mpc::ClientState & clientState, Mpc::LibraryEntry const * const entry, int32_t position)
{
   if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL))
   {
      if (position != -1)
      {
         client.Add(*(entry->song_), position);
      }
      else if ((Main::Settings::Instance().Get(Setting::AddPosition) == Setting::AddEnd) ||
               (clientState.GetCurrentSongPos() == -1))
      {
         client.Add(*(entry->song_));
      }
      else
      {
         client.Add(*(entry->song_), clientState.GetCurrentSongPos() + 1);
      }
   }
   else
   {
      int current = -1;

      if ((Main::Settings::Instance().Get(Setting::AddPosition) == Setting::AddNext) &&
          (clientState.GetCurrentSongPos() != -1))
      {
         current = clientState.GetCurrentSongPos() + 1;
      }

      for (auto child : entry->children_)
      {
         AddToPlaylist(client, clientState, child, current);

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
      }
   }
   else
   {
      for (auto child : entry->children_)
      {
         RemoveFromPlaylist(client, child);
      }
   }
}

void Library::ForEachChild(uint32_t index, FUNCTION<void (Mpc::Song *)> callback) const
{
   for (auto child : Get(index)->children_)
   {
      if (child->type_ == AlbumType)
      {
         for (auto child2 : child->children_)
         {
            (callback)(child2->song_);
         }
      }
      else if (child->type_ == SongType)
      {
         (callback)(child->song_);
      }
   }
}

void Library::ForEachChild(uint32_t index, FUNCTION<void (Mpc::LibraryEntry *)> callback) const
{
   for (auto child : Get(index)->children_)
   {
      if (child->type_ == AlbumType)
      {
         for (auto child2 : child->children_)
         {
            (callback)(child2);
         }
      }

      (callback)(child);
   }
}

void Library::ForEachSong(FUNCTION<void (Mpc::Song *)> callback) const
{
   for (uint32_t i = 0; i < Size(); ++i)
   {
      if (Get(i)->type_ == ArtistType)
      {
         for (auto child : Get(i)->children_)
         {
            if (child->type_ == AlbumType)
            {
               for (auto child2 : child->children_)
               {
                  (callback)(child2->song_);
               }
            }
         }
      }
   }
}

void Library::ForEachParent(FUNCTION<void (Mpc::LibraryEntry *)> callback) const
{
   for (uint32_t i = 0; i < Size(); ++i)
   {
      if (Get(i)->type_ == ArtistType)
      {
         for (auto child : Get(i)->children_)
         {
            if (child->type_ == AlbumType)
            {
               (callback)(child);
            }
         }

         (callback)(Get(i));
      }
   }
}


void Library::Expand(uint32_t line)
{
   uint32_t position = line;

   if ((Get(line)->expanded_ == false) && (Get(line)->type_ != Mpc::SongType))
   {
      Get(line)->expanded_ = true;

      for (auto child : Get(line)->children_)
      {
         Add(child, ++position);
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

      // Separated function out into variable as compile fails on g++ 4.7.2
      // if passed directly to function using the lambda
      FUNCTION<void (LibraryEntry *)> function = [this] (LibraryEntry * exp) { RemoveAndUnexpand(exp); };
      ForEachChild(Pos, function);
      entryToCollapse->expanded_ = false;
   }
}


std::string Library::String(uint32_t position) const
{
   Mpc::EntryType const type = Get(position)->type_;

   std::string Result = "";

   if (type == Mpc::ArtistType)
   {
      Result = Get(position)->artist_;
   }
   else if (type == Mpc::AlbumType)
   {
      Result = Get(position)->album_;
   }
   else if (type == Mpc::SongType)
   {
      Result = Get(position)->song_->FormatString(settings_.Get(Setting::LibraryFormat));
   }

   return Result;
}

std::string Library::PrintString(uint32_t position) const
{
   Mpc::EntryType const type = Get(position)->type_;

   std::string Result = "";

   if (type == Mpc::ArtistType)
   {
      Result = "$B " + Get(position)->artist_ + "$R$B";
   }
   else if (type == Mpc::AlbumType)
   {
      Result = "    " + Get(position)->album_;
   }
   else if (type == Mpc::SongType)
   {
      Result = "       " + Get(position)->song_->FormatString(settings_.Get(Setting::LibraryFormat));
   }

   return Result;
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

   if (entry == lastArtistEntry_)
   {
      lastArtistEntry_ = NULL;
   }

   if (entry == lastAlbumEntry_)
   {
      lastAlbumEntry_  = NULL;
      lastArtistEntry_ = NULL;
   }
}


void Mpc::MarkUnexpanded(LibraryEntry * const entry)
{
   entry->expanded_ = false;
}

/* vim: set sw=3 ts=3: */
