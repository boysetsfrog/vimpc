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
#include "algorithm.hpp"
#include "buffer.hpp"
#include "settings.hpp"

// Lists
namespace Mpc
{
   class List
   {
      public:
         List(std::string const & name) : path_(name), name_(name), file_(false) { }
         List(std::string const & path, std::string const & name) : path_(path), name_(name), file_(true) { }

         bool operator!=(Mpc::List const & rhs) const
         {
            return ((this->path_ != rhs.path_) ||
                    (this->name_ != rhs.name_));
         }

         std::string path_;
         std::string name_;
         bool        file_;
   };

   class ListComparator
   {
      public:
      bool operator() (List i, List j)
      {
         return Algorithm::icompare(i.name_, j.name_,
                  Main::Settings::Instance().Get(Setting::IgnoreTheSort),
                  Main::Settings::Instance().Get(Setting::IgnoreCaseSort));
      }
   };


   class Lists : public Main::Buffer<List>
   {
   public:
      Lists()
      {
      }
      ~Lists()
      {
      }

   public:
      using Main::Buffer<List>::Sort;

      void Sort()
      {
         ListComparator sorter;
         Main::Buffer<List>::Sort(sorter);
      }

      std::string String(uint32_t position) const      { return Get(position).name_; }
      std::string PrintString(uint32_t position) const { return " " + Get(position).name_; }
   };
}
#endif
/* vim: set sw=3 ts=3: */
