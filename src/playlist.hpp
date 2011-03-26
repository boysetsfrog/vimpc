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

   playlist.hpp - handling of the mpd playlist interface 
   */

#ifndef __UI__PLAYLIST
#define __UI__PLAYLIST

// Includes
#include <vector>
#include "song.hpp"

// Playlist 
namespace Mpc
{
   class Playlist : private std::vector<Song *>
   {
   public:
      static Playlist & Instance()
      {
         static Playlist * instance = NULL;

         if (instance == NULL)
         {
            instance = new Playlist();
         }

         return *instance;
      }

   private:
      Playlist()  {} 
      ~Playlist() {}

   public:
      Mpc::Song const * Song(uint32_t position) const;
      void Add(Mpc::Song * const song);
      void Add(Mpc::Song * const song, uint32_t position);
      void Remove(uint32_t position, uint32_t count = 1);
      void Clear();
      uint32_t Songs() const;
   };
}
#endif
