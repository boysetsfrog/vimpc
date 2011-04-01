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

#include "buffer.hpp"
#include "song.hpp"

#include <vector>

namespace Main { class Settings; }
namespace Ui   { class LibraryWindow; }

namespace Mpc
{
   class  Client;
   class  Library;
   class  LibraryEntry;

   typedef std::vector<LibraryEntry *> LibraryEntryVector;

   typedef enum
   {
      ArtistType = 0,
      AlbumType,
      SongType
   } EntryType;

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
         parent_  ()
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

         if (song_ == NULL)
         {
            comparison = ((artist_ < rhs.artist_) || (album_ < rhs.album_));
         }

         if ((song_ != NULL) && (rhs.song_ != NULL) && (comparison == false))
         {
            comparison = (song_->Track() < rhs.song_->Track());
         }

         return comparison;
      }

   public:
      ~LibraryEntry()
      {
         if (expanded_ == false)
         {
            for (LibraryEntryVector::iterator it = children_.begin(); it != children_.end(); ++it)
            {
               delete *it;
            }
         }

         children_.clear();

         delete song_;
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
   };


   // Library class
   class Library : public Main::Buffer<LibraryEntry *>
   {
   public:
      using Main::Buffer<LibraryEntry *>::Sort;
      using Main::Buffer<LibraryEntry *>::Add;

      static Library & Instance()
      {
         static Library * instance = NULL;

         if (instance == NULL)
         {
            instance = new Library();
         }

         return *instance;
      }

   private:
      Library()  {}
      ~Library() {}

   public:
      Mpc::Song * Song(Mpc::Song const * const song);

      void Sort();
      void Sort(LibraryEntry * entry);
      void Add(Mpc::Song const * const song);
      void AddToPlaylist(Mpc::Client & client, Mpc::Song::SongCollection Collection, uint32_t position);

   public:
      void Expand(uint32_t line);
      void Collapse(uint32_t line);

   private:
      void AddToPlaylist(Mpc::Client & client, Mpc::LibraryEntry const * const entry);

      typedef Main::CallbackObject<Mpc::Library, Library::BufferType> CallbackObject;
   };

}

#endif
