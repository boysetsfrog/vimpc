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

   browse.hpp - handling of the mpd playlist interface 
   */

#ifndef __MPC__BROWSE
#define __MPC__BROWSE

// Includes
#include "callback.hpp"
#include "song.hpp"

#include "buffer/buffer.hpp"

// Browse 
namespace Mpc
{
   class Client;

   class Browse : public Main::Buffer<Mpc::Song *>
   {
   private:
      typedef Main::CallbackObject<Mpc::Browse, Browse::BufferType> CallbackObject;
      typedef Main::CallbackFunction<Browse::BufferType> CallbackFunction; 

   public:
      Browse(bool IncrementReferences = false);
      ~Browse();

   public:
   void AddToPlaylist(Mpc::Client & client, uint32_t position);
   void Sort();

   private:
      class BrowseComparator
      {
         public:
         bool operator() (Mpc::Song * i, Mpc::Song * j) { return (*i<*j); };
      };
   };
}

#endif
