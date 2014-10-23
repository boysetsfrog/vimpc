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

   library.hpp - handling of the mpd music library
   */

#ifndef __UI__LIBRARY
#define __UI__LIBRARY

#include "algorithm.hpp"
#include "buffer.hpp"
#include "settings.hpp"
#include "song.hpp"

#include <vector>

namespace Ui   { class LibraryWindow; }

namespace Mpc
{
   class  Client;
   class  ClientState;
   class  Library;
   class  LibraryEntry;

   typedef std::vector<LibraryEntry *> LibraryEntryVector;

   class LibraryEntry
   {
   public:
      friend class Library;

      LibraryEntry() :
         type_    (SongType),
         artist_  (""),
         album_   (""),
         song_    (NULL),
         expanded_(false),
         children_(),
         parent_  (NULL),
         childrenInPlaylist_(0),
         partial_(0)
      { }

   public:
      bool operator==(LibraryEntry const & rhs) const
      {
         return (!((*this) < rhs) && !(rhs < (*this)));
      }

      bool operator!=(LibraryEntry const & rhs) const
      {
         return (((*this) < rhs) || (rhs < (*this)));
      }

      bool operator<(LibraryEntry const & rhs) const
      {
         bool comparison = false;

         Main::Settings const & settings = Main::Settings::Instance();

         if (type_ == ArtistType)
         {
            comparison = Algorithm::icompare(artist_, rhs.artist_, settings.Get(Setting::IgnoreTheSort), settings.Get(Setting::IgnoreCaseSort));
         }
         else if (type_ == AlbumType)
         {
            comparison = Algorithm::icompare(album_, rhs.album_, settings.Get(Setting::IgnoreTheSort), settings.Get(Setting::IgnoreCaseSort));
         }
         else if ((song_ != NULL) && (rhs.song_ != NULL))
         {
            uint32_t track    = atoi(song_->Track().c_str());
            uint32_t rhsTrack = atoi(rhs.song_->Track().c_str());
            uint32_t disc     = atoi(song_->Disc().c_str());
            uint32_t rhsDisc  = atoi(rhs.song_->Disc().c_str());
            comparison = ((track < rhsTrack) && (disc <= rhsDisc));
         }

         return comparison;
      }

   public:
      ~LibraryEntry()
      {
         if (song_ != NULL)
         {
            song_->SetEntry(NULL);
            delete song_;
         }

         song_ = NULL;

         FOREACH(auto child, children_)
         {
            if ((child) && (child->Parent() == this))
            {
               delete child;
            }
         }

         children_.clear();
      }

      void AddedToPlaylist()
      {
         ++childrenInPlaylist_;

         if ((parent_ != NULL) && (childrenInPlaylist_ == 1))
         {
            parent_->AddPartial();
         }

         if ((parent_ != NULL) && ((childrenInPlaylist_ == static_cast<int32_t>(children_.size())) || (type_ == Mpc::SongType)))
         {
            parent_->AddedToPlaylist();
         }
      }

      void RemovedFromPlaylist()
      {
         if ((parent_ != NULL) && ((childrenInPlaylist_ == static_cast<int32_t>(children_.size())) || (type_ == Mpc::SongType)))
         {
            parent_->RemovedFromPlaylist();
         }

         if (childrenInPlaylist_ > 0)
         {
            --childrenInPlaylist_;
         }

         if ((parent_ != NULL) && (childrenInPlaylist_ == 0))
         {
            parent_->RemovePartial();
         }
      }

      void AddPartial()
      {
         ++partial_;

         if (parent_ != NULL)
         {
            parent_->AddPartial();
         }
      }

      void RemovePartial()
      {
         if (partial_ > 0)
         {
            --partial_;
         }

         if (parent_ != NULL)
         {
            parent_->RemovePartial();
         }
      }

   public:
      LibraryEntry * Parent()
      {
         return parent_;
      }

      uint32_t InPlaylistCount()
      {
         return childrenInPlaylist_;
      }

   private:
      LibraryEntry(LibraryEntry & entry);
      LibraryEntry & operator=(LibraryEntry & entry);

   private:
      class LibraryComparator
      {
         public:
         bool operator() (LibraryEntry * i, LibraryEntry * j) { return (*i<*j); };
      };

   public:
      EntryType          type_;
      std::string        artist_;
      std::string        album_;
      Mpc::Song *        song_;
      bool               expanded_;
      LibraryEntryVector children_;
      LibraryEntry *     parent_;
      int32_t            childrenInPlaylist_;
      int32_t            partial_;
   };


   // Library class
   class Library : public Main::Buffer<LibraryEntry *>
   {
   public:
      using Main::Buffer<LibraryEntry *>::Sort;
      using Main::Buffer<LibraryEntry *>::Add;

      Library();
      ~Library();

   public:
      Mpc::Song * Song(std::string uri) const;

      void Clear(bool Delete = true);
      void Sort();
      void Sort(LibraryEntry * entry);
      void Add(Mpc::Song * song);
      void AddToPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, Mpc::ClientState & clientState, uint32_t position);
      void RemoveFromPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, Mpc::ClientState & clientState, uint32_t position);

      void CreateVariousArtist();
      Mpc::LibraryEntry * CreateArtistEntry(std::string artist);
      Mpc::LibraryEntry * CreateAlbumEntry(Mpc::Song * song);

      void ForEachChild(uint32_t index, FUNCTION<void (Mpc::Song *)> callback) const;
      void ForEachChild(uint32_t index, FUNCTION<void (Mpc::LibraryEntry *)> callback) const;
      void ForEachSong(FUNCTION<void (Mpc::Song *)> callback) const;
      void ForEachParent(FUNCTION<void (Mpc::LibraryEntry *)> callback) const;

   public:
      void Expand(uint32_t line);
      void Collapse(uint32_t line);

      std::string String(uint32_t position) const;
      std::string PrintString(uint32_t position) const;

   private:
      void RecreateLibraryFromURIs();

      void AddToPlaylist(Mpc::Client & client, Mpc::ClientState & clientState, Mpc::LibraryEntry const * const entry, int32_t position = -1);
      void RemoveFromPlaylist(Mpc::Client & client, Mpc::LibraryEntry const * const entry);
      void DeleteEntry(LibraryEntry * const entry);
      void CheckIfVariousRemoved(LibraryEntry * const entry);
      void RemoveAndUnexpand(LibraryEntry * const entry);

   private:
      Main::Settings & settings_;
      std::map<std::string, Mpc::Song *> uriMap_;
      Mpc::LibraryEntry * variousArtist_;
      Mpc::LibraryEntry * lastAlbumEntry_;
      Mpc::LibraryEntry * lastArtistEntry_;
   };

   //Flag a library entry as not expanded, this does not actually collapse it however
   void MarkUnexpanded(LibraryEntry * const entry);
}

#endif
/* vim: set sw=3 ts=3: */
