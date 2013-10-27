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
#include "settings.hpp"
#include "song.hpp"

#include "buffer/buffer.hpp"

// Browse
namespace Mpc
{
   class Client;

   class Browse : public Main::Buffer<Mpc::Song *>
   {
   public:
      Browse(bool IncrementReferences = false);
      ~Browse();

      std::string String(uint32_t position) const;
      std::string PrintString(uint32_t position) const;

   private:
      Main::Settings const & settings_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
