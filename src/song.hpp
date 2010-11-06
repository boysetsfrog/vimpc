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
      Song(uint32_t id) :
         id_(id)
      { }

      ~Song()
      { }

   public:
      int32_t Id()
      {
         return id_;
      }

      void SetArtist(const char * artist)
      {
         if (artist != NULL)
         {
            artist_ = artist;
         }
         else
         {
            artist_ = "Unknown";
         }
      }

      std::string & Artist()
      {
         return artist_;
      }

      void SetTitle(const char * title)
      {
         if (title != NULL)
         {
            title_ = title;
         }
         else
         {
            title_ = "Unknown";
         }
      }

      std::string & Title()
      {
         return title_;
      }

      std::string SongInformation()
      {
         std::string songInformation;

         songInformation += artist_;
         songInformation += title_;

         return songInformation;
      }

   private:
      int32_t     id_;
      std::string artist_;
      std::string title_;
   };
}

#endif
