/*
   Vimpc
   Copyright (C) 2010 - 2011 Nathan Sweetman

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

   song.hpp - represents information of an individual track
   */

#ifndef __MPC_SONG
#define __MPC_SONG

#include <stdint.h>
#include <string>
#include <mpd/song.h>

#include <vector>
#include <map>

namespace Mpc
{
   class LibraryEntry;

   typedef enum
   {
      ArtistType = 0,
      AlbumType,
      PathType,
      PlaylistType,
      SongType
   } EntryType;

   class Song
   {
   public:
      Song();
      Song(Song const & song);
      ~Song();

   public:
      typedef std::string const & (Mpc::Song::*SongInformationFunction)() const;

      typedef enum
      {
         Single,
         All
      } SongCollection;

   public:
      // When we equate songs we can just check if they refer
      // to the same file in the database, if they do, they are the same
      bool operator==(Song const & rhs)
      {
         return (this->uri_ == rhs.URI());
      }

      bool operator!=(Song const & rhs)
      {
         return (this->uri_ != rhs.URI());
      }

      bool operator==(mpd_song const & rhs)
      {
         return (this->uri_ == (mpd_song_get_uri(&rhs)));
      }

      bool operator!=(mpd_song const & rhs)
      {
         return (this->uri_ != (mpd_song_get_uri(&rhs)));
      }

      // Sort by artist then title
      bool operator<(Song const & rhs) const
      {
         return ((artist_ < rhs.artist_) || (title_ < rhs.title_));
      }

   public:
      int32_t Reference() const;

      // Find the corresponding entry in the library
      // for this song and update it's reference count
      static void IncrementReference(Song * song);
      static void DecrementReference(Song * song);
      static void SwapThe(std::string & String);

      void SetArtist(const char * artist);
      std::string const & Artist() const;

      void SetAlbumArtist(const char * artist);
      std::string const & AlbumArtist() const;

      void SetAlbum(const char * album);
      std::string const & Album() const;

      void SetTitle(const char * title);
      std::string const & Title() const;

      void SetTrack(const char * track);
      std::string const & Track() const;

      void SetURI(const char * uri);
      std::string const & URI() const;

      void SetGenre(const char * genre);
      std::string const & Genre() const;

      void SetDate(const char * date);
      std::string const & Date() const;

      void SetDisc(const char * disc);
      std::string const & Disc() const;

      void SetDuration(int32_t duration);
      int32_t Duration() const;
      std::string const & DurationString() const;

      void SetVirtualEnd(int32_t end);
      int32_t VirtualEnd() const;

      void SetEntry(LibraryEntry * entry);
      LibraryEntry * Entry() const;

      std::string FormatString(std::string fmt) const;
      std::string ParseString(std::string::const_iterator &it, bool valid) const;

   public:
      typedef std::string const & (Mpc::Song::*SongFunction)() const;
      static std::map<char, SongFunction> SongInfo;
      static void RepopulateSongFunctions();

   private:
      static std::vector<std::string> Artists;
      static std::map<std::string, uint32_t> ArtistMap;

      static std::vector<std::string> Albums;
      static std::map<std::string, uint32_t> AlbumMap;

      static std::vector<std::string> Tracks;
      static std::map<std::string, uint32_t> TrackMap;

      static std::vector<std::string> Genres;
      static std::map<std::string, uint32_t> GenreMap;

      static std::vector<std::string> Dates;
      static std::map<std::string, uint32_t> DateMap;

      static std::vector<std::string> Discs;
      static std::map<std::string, uint32_t> DiscMap;

   private:
      void Set(const char * newVal, int32_t & oldVal, std::vector<std::string> & Values, std::map<std::string, uint32_t> & Indexes);

   private:
      int32_t     reference_;
      int32_t     artist_;
      int32_t     albumArtist_;
      int32_t     album_;
      int32_t     track_;
      int32_t     genre_;
      int32_t     date_;
      int32_t     disc_;
      int32_t     duration_;
      int32_t     virtualEnd_;
      std::string uri_;
      std::string title_;

      mutable std::string lastFormat_;
      mutable std::string formatted_;

      LibraryEntry * entry_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
