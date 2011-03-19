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

   song.hpp - represents information of an individual track
   */

#ifndef __MPC_SONG
#define __MPC_SONG

#include <stdint.h>
#include <string>

namespace Mpc
{
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
      bool operator==(Song const & rhs)
      {
         return (this->uri_ == rhs.URI());
      }

   public:
      int32_t Reference() const;
      void IncrementReference();
      void DecrementReference();

      void SetArtist(const char * artist);
      std::string const & Artist() const;
      
      void SetAlbum(const char * album);
      std::string const & Album() const;

      void SetTitle(const char * title);
      std::string const & Title() const;

      void SetTrack(const char * track);
      std::string const & Track() const;

      void SetURI(const char * uri);
      std::string const & URI() const;

      void SetDuration(int32_t duration);
      int32_t Duration() const;
      std::string DurationString() const;

      std::string PlaylistDescription() const;
      std::string FullDescription()     const;

   private:
      int32_t     reference_;
      std::string artist_;
      std::string album_;
      std::string title_;
      std::string track_;
      std::string uri_;
      int32_t     duration_;
   };
}

#endif
