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

   window.cpp - class representing an ncurses window
   */

#include "window.hpp"

#include <iostream>

#include "screen.hpp"

using namespace Ui;

Window::Window(Ui::Screen const & screen) :
   screen_    (screen),
   window_    (NULL),
   scrollLine_(screen_.MaxRows()),
   autoScroll_(false)
{
   window_ = newwin(screen_.MaxRows(), screen_.MaxColumns(), 0, 0);
}

Window::Window(Ui::Screen const & screen, int h, int w, int x, int y) :
   screen_    (screen),
   window_    (NULL),
   scrollLine_(h),
   autoScroll_(false)
{
   window_ = newwin(h, w, x, y);
}

Window::~Window()
{
   delwin(window_);
}

void Window::Confirm() const
{
}

void Window::Scroll(int32_t scrollCount)
{
   uint16_t const newLine = (scrollLine_ + scrollCount);

   if (newLine < screen_.MaxRows())
   {
      scrollLine_ = screen_.MaxRows();
   }
   else if (newLine > BufferSize())
   {
      scrollLine_ = BufferSize();
   }
   else
   {
      scrollLine_ = newLine;
   }
}

void Window::ScrollTo(uint16_t scrollLine)
{
   scrollLine_ = scrollLine + (screen_.MaxRows() / 2);

   if (scrollLine_ < screen_.MaxRows())
   {
      scrollLine_ = screen_.MaxRows();
   }
   else if (scrollLine_ > BufferSize())
   {
      scrollLine_ = BufferSize();
   }
}

void Window::Search(std::string const & searchString) const
{

}

void Window::Redraw()
{

}

bool Window::Select(Position position, uint32_t count)
{
   if (position == Window::First)
   {
      int32_t scroll = FirstLine() + count;

      if (scroll > (int32_t) LastLine()) 
      { 
         scroll = LastLine(); 
      }

      ScrollTo(scroll);
   }
   else if (position == Window::Last)
   {
      int32_t scroll = LastLine() - count + 1;

      if (scroll < (int32_t) FirstLine() + 1) 
      { 
         scroll = FirstLine() + 1; 
      }

      ScrollTo(scroll);
   }
   else if (position == Window::Middle)
   {
      ScrollTo(FirstLine() + ((LastLine() - FirstLine() + 1) / 2) );
   }

   return true;
}


void Window::Erase()
{
   werase(N_WINDOW());
}

void Window::Refresh()
{
   wnoutrefresh(N_WINDOW());
}

void Window::SetAutoScroll(bool autoScroll)
{
   autoScroll_ = autoScroll;
}

bool Window::AutoScroll() const
{
   return autoScroll_;
}


uint16_t Window::FirstLine() const
{
   uint16_t result = 0;

   if ((scrollLine_ - screen_.MaxRows()) > 0)
   {
      result = (scrollLine_ - screen_.MaxRows());
   }

   return result;
}

void Window::ResetScroll()
{
   scrollLine_ = screen_.MaxRows();
}

uint16_t Window::ScrollLine() const
{
   return scrollLine_;
}

void Window::SetScrollLine(uint16_t scrollLine)
{
   scrollLine_ = scrollLine;
}
