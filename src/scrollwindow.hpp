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

   scrollwindow.hpp - class representing a window that can scroll
   */

#ifndef __UI__SCROLLWINDOW
#define __UI__SCROLLWINDOW

#include "attributes.hpp"
#include "window.hpp"

namespace Ui
{
   class Screen;

   class ScrollWindow : public Window
   {
   public:
      ScrollWindow(Ui::Screen const & screen);
      virtual ~ScrollWindow();

   public:
      typedef enum 
      { 
         First,
         Middle,
         Last,
         PositionCount
      } Position;

   public:
      virtual void Print(uint32_t line) const = 0;
      virtual void Scroll(int32_t scrollCount);
      virtual void ScrollTo(uint16_t scrollLine);

   public:
      bool Select(Position position, uint32_t count);

   public:
      void SetAutoScroll(bool autoScroll);
      bool AutoScroll() const;

   public:
      virtual std::string SearchPattern(UNUSED int32_t id) { return ""; }

   public:
      uint32_t FirstLine()   const;
      uint32_t LastLine()    const { return ScrollLine(); }
      uint32_t ContentSize() const { return BufferSize() - 1; }

      virtual  uint16_t CurrentLine() const { return FirstLine(); }

   protected:
      void ResetScroll(); 
      void SetScrollLine(uint16_t scrollLine);
      uint16_t ScrollLine() const;

   protected:
      virtual size_t BufferSize() const = 0;

   protected:
      Ui::Screen const & screen_;
      uint16_t   scrollLine_;
      bool       autoScroll_;
   };
}

#endif
