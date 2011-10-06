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

   infowindow.hpp - class representing a window that can scroll
   */

#ifndef __UI__INFOWINDOW
#define __UI__INFOWINDOW

#include "song.hpp"
#include "scrollwindow.hpp"

namespace Ui
{
   class Screen;

   class InfoWindow : public ScrollWindow
   {
   public:
      InfoWindow(Mpc::Song * song, Ui::Screen & screen, std::string name = "Unknown");
      InfoWindow(Ui::Screen & screen, std::string name = "Unknown");
      virtual ~InfoWindow();

   public:
      void Print(uint32_t line) const;
      uint32_t Current() const { return CurrentLine(); };
      std::string SearchPattern(int32_t id) { return ""; }

   protected:
      size_t BufferSize() const
      {
         if (song_ != NULL)
         {
            return 1;
         }

         return 0;
      }

   private:
      Mpc::Song * song_;
   };
}

#endif
