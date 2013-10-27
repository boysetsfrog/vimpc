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

   browse.cpp - handling of the mpd playlist interface
   */

// Includes
#include "buffers.hpp"
#include "mpdclient.hpp"
#include "buffer/browse.hpp"

// Browse
using namespace Mpc;

Browse::Browse(bool IncrementReferences) :
   settings_(Main::Settings::Instance())
{
   if (IncrementReferences == true)
   {
      AddCallback(Main::Buffer_Add,    [] (Mpc::Song * song) { Mpc::Song::IncrementReference(song); });
      AddCallback(Main::Buffer_Remove, [] (Mpc::Song * song) { Mpc::Song::DecrementReference(song); });
   }
}

Browse::~Browse()
{
}

std::string Browse::String(uint32_t position) const
{
    return Get(position)->FormatString(settings_.Get(Setting::SongFormat));
}

std::string Browse::PrintString(uint32_t position) const
{
   std::string out("");

   if (position < Size())
   {
      if (settings_.Get(Setting::BrowseNumbers) == true)
      {
         out += "$H[$I$L$D]$H ";
      }
      else
      {
         out = " ";
      }

      out += Get(position)->FormatString(settings_.Get(Setting::SongFormat));
   }

   return out;
}


/* vim: set sw=3 ts=3: */
