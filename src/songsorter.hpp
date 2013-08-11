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
         settings_(Main::Settings::Instance()),
         format_  (sortFormat)
      {
      }

      public:
      bool operator() (Mpc::Song * i, Mpc::Song * j) 
      { 
         // Sort based on print format
         if (format_ == "format")
         {
            return (i->FormatString(settings_.Get(Setting::SongFormat)) < j->FormatString(settings_.Get(Setting::SongFormat)));
         }

         // Sort based on position in library
         else if (format_ == "library")
         {
            Mpc::LibraryEntry * iE = i->Entry();
            Mpc::LibraryEntry * jE = j->Entry();

            if ((iE != NULL) && (jE != NULL))
            {
               Mpc::LibraryEntry * iEp = iE->parent_;
               Mpc::LibraryEntry * jEp = jE->parent_;

               if ((iEp != NULL) && (jEp != NULL))
               {
                  Mpc::LibraryEntry * iEpp = iEp->parent_;
                  Mpc::LibraryEntry * jEpp = jEp->parent_;

                  if (iEpp != jEpp)
                  {
                     return *iEpp < *jEpp;
                  }
                  else if (iEp != jEp)
                  {
                     return *iEp < *jEp;
                  }
                  else
                  {
                     return *iE < *jE;
                  }
               }
            }

            Debug("No valid library entries during sort");
         }

         return (i->FormatString(settings_.Get(Setting::SongFormat)) < j->FormatString(settings_.Get(Setting::SongFormat)));
      };

      private:
         Main::Settings const & settings_;
         std::string    const   format_;
   };
}
