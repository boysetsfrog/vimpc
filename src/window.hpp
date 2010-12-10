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

   window.hpp - class representing an ncurses window
   */

#ifndef __UI__WINDOW
#define __UI__WINDOW

#include <ncursesw/ncurses.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace Ui
{
   class Window
   {
   public:
      Window(int h, int w, int x, int y);
      virtual ~Window();

   private:
      Window(Window & window);
      Window & operator=(Window & window);

   public:
      virtual void Print(uint32_t line) const = 0;
      virtual void Confirm() const;
      virtual void Redraw();

   public:
      void Erase();
      void Refresh();

   public:
      uint32_t ContentSize() const { return BufferSize() - 1; }

   protected:
      WINDOW * N_WINDOW() const { return window_; }

   private:
      virtual size_t BufferSize() const = 0;

   private:
      WINDOW * const window_;
   };
}

#endif
