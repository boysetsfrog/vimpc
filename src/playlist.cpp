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

   playlist.cpp - handling of the mpd playlist interface 
   */

#include "playlist.hpp"

using namespace Mpc;

Mpc::Song const * Playlist::Song(uint32_t position) const
{
   Mpc::Song const * song = NULL;

   if (position < size())
   {
      song = at(position);
   }

   return song; 
}

void Playlist::Add(Mpc::Song * const song)
{
   if (song != NULL)
   {
      song->IncrementReference();
      insert(end(), song);
   }
}

void Playlist::Add(Mpc::Song * const song, uint32_t position)
{
   if (song != NULL)
   {
      if (position == size())
      {
         Add(song);
      }
   }
}

void Playlist::Remove(uint32_t position, uint32_t count)
{
   iterator it = begin(); 

   for (uint32_t id = 0; ((id != position) && (it != end())); ++id)
   {
      it++;
   }

   if (it != end())
   {
      for (int i = count; (i > 0) && (it != end()); --i)
      {
         (*it)->DecrementReference();
         it = erase(it);
      }
   }
}

void Playlist::Clear()
{
   // Flag all the songs in the playlist as no longer in the playlist
   for (iterator it = begin(); it != end(); ++it)
   {
      (*it)->DecrementReference();
   }

   clear();
}

uint32_t Playlist::Songs() const 
{ 
   return size(); 
}
