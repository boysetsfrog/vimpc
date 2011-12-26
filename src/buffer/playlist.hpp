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

#ifndef __MPC__PLAYLIST
#define __MPC__PLAYLIST

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
      typedef Main::CallbackObject<Mpc::Playlist, Playlist::BufferType> CallbackObject;
      typedef Main::CallbackFunction<Playlist::BufferType> CallbackFunction;

   public:
      Playlist(bool IncrementReferences = false)
      {
         if (IncrementReferences == true)
         {
            AddCallback(Main::Buffer_Add,    new CallbackFunction(&Mpc::Song::IncrementReference));
            AddCallback(Main::Buffer_Remove, new CallbackFunction(&Mpc::Song::DecrementReference));
         }
      }
      ~Playlist() {}
   };
}
#endif
/* vim: set sw=3 ts=3: */
