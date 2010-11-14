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

#include <ncurses.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace Ui
{
   class Screen;

   class Window
   {
   public:
      Window(Ui::Screen const & screen);
      Window(Ui::Screen const & screen, int h, int w, int x, int y);
      virtual ~Window();

   public:
      virtual void Print(uint32_t line) const = 0;
      virtual void Confirm() const;
      virtual void Scroll(int32_t scrollCount);
      virtual void ScrollTo(uint16_t scrollLine);
      virtual void Search(std::string const & searchString) const;
      virtual void Redraw();

   public:
      void Erase();
      void Refresh();

   public:
      void SetAutoScroll(bool autoScroll);
      bool AutoScroll() const;

   public:
      uint16_t FirstLine()   const;
      virtual  uint16_t CurrentLine() const { return FirstLine(); }
      uint32_t LastLine()    const { return BufferSize() - 1; }

   protected:
      void ResetScroll(); 
      void SetScrollLine(uint16_t scrollLine);
      uint16_t ScrollLine() const;

   protected:
      WINDOW * const N_WINDOW() const { return window_;}

   private:
      virtual size_t BufferSize() const = 0;

   protected:
      Ui::Screen const & screen_;
      WINDOW   * window_;
      uint16_t    scrollLine_;
      bool       autoScroll_;
   };
}

#endif
