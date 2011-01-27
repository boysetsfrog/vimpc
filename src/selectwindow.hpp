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

   selectwindow.hpp - window that is scrollable and has selectable elements 
   */

#ifndef __UI__SELECTWINDOW
#define __UI__SELECTWINDOW

#include "song.hpp"
#include "scrollwindow.hpp"

namespace Ui
{
   class SelectWindow : public Ui::ScrollWindow
   {
   public:
      SelectWindow(Ui::Screen const & screen);
      ~SelectWindow();

   public:
      void Scroll(int32_t scrollCount);
      void ScrollTo(uint16_t scrollLine);

      uint16_t CurrentLine() const;

   protected:
      int64_t LimitCurrentSelection(int64_t currentSelection) const;

      int64_t currentSelection_;
   };
}

#endif
