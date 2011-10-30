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
   typedef std::pair<int64_t, int64_t> Selection;

   class SelectWindow : public Ui::ScrollWindow
   {
   public:

   public:
      SelectWindow(Ui::Screen & screen, std::string name = "Unknown");
      ~SelectWindow();

   public:
      void Resize(int rows, int columns);
      void Scroll(int32_t scrollCount);
      void ScrollTo(uint16_t scrollLine);

      uint16_t CurrentLine() const;
      void Confirm();

   public: // Ui::ScrollWindow
      void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void Escape();
      void Visual();

   public:
      bool IsSelected(uint32_t line) const;
      Selection CurrentSelection() const;

   protected:
      void LimitCurrentSelection() const;

   private:
      bool              visualMode_;
      mutable int64_t   currentLine_;
      mutable Selection currentSelection_;
   };
}

#endif
