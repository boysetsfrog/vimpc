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

#include "selectwindow.hpp"
#include "song.hpp"

#include <map>

namespace Main
{
   class Settings;
}

namespace Mpc
{
   class Client;
}

namespace Ui
{
   class Search;

   class LibraryWindow : public Ui::SelectWindow
   {
   public:
      LibraryWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search);
      ~LibraryWindow();

   private:
      LibraryWindow(LibraryWindow & library);
      LibraryWindow & operator=(LibraryWindow & library);

   public:
      void AddSong(Mpc::Song const * const song);

   public:
      void Print(uint32_t line) const;
      void Left(Ui::Player & player, uint32_t count);
      void Right(Ui::Player & player, uint32_t count);
      void Confirm();
      int32_t AddSongs();
      void Redraw();

   public:
      std::string SearchPattern(int32_t id);

   private:
      void Expand(uint32_t line);
      void Collapse(uint32_t line);
      void Clear();
      void PopulateBuffer();

   private:
      size_t BufferSize() const { return buffer_.size(); }

   private:
      struct LibraryEntry;
      struct LibraryHeirachyEntry;

      typedef std::map<std::string, LibraryHeirachyEntry *>  Library;
      typedef std::vector<LibraryEntry *> SongBuffer;

   private:
      int32_t DetermineSongColour(LibraryEntry const * const entry) const;

   private:
      typedef enum
      {
         ArtistType = 0,
         AlbumType,
         SongType
      } EntryType;

      class LibraryEntry
      {
      public:
         LibraryEntry() :
            type_    (SongType),
            artist_  (""),
            album_   (""),
            song_    (NULL),
            expanded_(false)
         { }

      private:
         LibraryEntry(LibraryEntry & entry);
         LibraryEntry & operator=(LibraryEntry & entry);

      public:
         EntryType   type_;
         std::string artist_;
         std::string album_;
         Mpc::Song * song_;
         bool        expanded_;
      };

      class LibraryHeirachyEntry
      {
      public:
         LibraryHeirachyEntry() :
            libraryEntry_(),
            children_    ()
         { }

      public:
         LibraryEntry libraryEntry_;
         Library      children_;
      };


      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Ui::Search     const & search_;

      // Maintains the current tree heirachy of the library
      Library * library_;

      // Used to actually print the entries
      SongBuffer buffer_;
   };
}

#endif
