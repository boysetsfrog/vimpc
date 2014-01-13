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

   songsorter.hpp - sort songs based on a given criterium
*/

#include <song.hpp>
#include <buffer/library.hpp>
#include <window/debug.hpp>

//! \TODO This is really really ugly and needs work as well as more options
//!       preferably a csv style "artist,album,track" etc format
//!       rather than just "format" or "library"
namespace Ui
{
   class SongSorter
   {
      public:
      SongSorter(std::string sortFormat) :
         settings_  (Main::Settings::Instance()),
         format_    (sortFormat),
         ignoreCase_(settings_.Get(Setting::IgnoreCaseSort)),
         ignoreThe_ (settings_.Get(Setting::IgnoreTheSort))
      {

      }

      public:
      bool operator() (Mpc::Song * i, Mpc::Song * j)
      {
         // Sort based on print format
         if (format_ == "format")
         {
            return Algorithm::icompare(i->FormatString(settings_.Get(Setting::SongFormat)), j->FormatString(settings_.Get(Setting::SongFormat)), ignoreThe_, ignoreCase_);
         }

        return Algorithm::icompare(i->FormatString(settings_.Get(Setting::SongFormat)), j->FormatString(settings_.Get(Setting::SongFormat)), ignoreThe_, ignoreCase_);
      };

      private:
         Main::Settings const & settings_;
         std::string    const   format_;
         bool           const   ignoreCase_;
         bool           const   ignoreThe_;
   };
}
