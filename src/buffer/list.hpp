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

   list.hpp - handling of multiple mpd playlists
   */

#ifndef __MPC__LISTS
#define __MPC__LISTS

// Includes
#include "buffer.hpp"
#include "callback.hpp"

// Lists
namespace Mpc
{
   class ListComparator
   {
      public:
      bool operator() (std::string i, std::string j) { return (i<j); };
   };

   class Lists : public Main::Buffer<std::string>
   {
   private:
      typedef Main::CallbackObject<Mpc::Lists, Lists::BufferType> CallbackObject;
      typedef Main::CallbackFunction<Lists::BufferType> CallbackFunction;

   public:
      Lists()
      {
      }
      ~Lists()
      {
      }

   public:
      using Main::Buffer<std::string>::Sort;

      void Sort()
      {
         ListComparator sorter;
         Main::Buffer<std::string>::Sort(sorter);
      }
   };
}
#endif
/* vim: set sw=3 ts=3: */
