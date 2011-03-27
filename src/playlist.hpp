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
#include "buffer.hpp"
#include "callback.hpp"
#include "song.hpp"

// Playlist 
namespace Mpc
{
   class Playlist : public Main::Buffer<Mpc::Song *>
   {
   private:
      typedef Main::Buffer<Mpc::Song *> BufferType;
      typedef Main::CallbackObject<Mpc::Playlist, Playlist::BufferParameter> Callback;

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
      Playlist()  
      {
         AddCallback(Main::Buffer_Add,    new Callback(*this, &Mpc::Playlist::IncrementReference));
         AddCallback(Main::Buffer_Remove, new Callback(*this, &Mpc::Playlist::DecrementReference));
      } 
      ~Playlist() {}

   public:
      void IncrementReference(Mpc::Song * & song) { song->IncrementReference(); }
      void DecrementReference(Mpc::Song * & song) { song->DecrementReference(); }
   };
}
#endif
