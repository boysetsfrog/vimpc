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

   scrollwindow.cpp - class representing a scrollable ncurses window
   */

#include "scrollwindow.hpp"

#include <iostream>

#include "screen.hpp"

using namespace Ui;

ScrollWindow::ScrollWindow(Ui::Screen const & screen) :
   Window     (screen.MaxRows() - 1, screen.MaxColumns(), 1, 0),
   screen_    (screen),
   scrollLine_(screen.MaxRows() - 1),
   autoScroll_(false)
{
}

ScrollWindow::~ScrollWindow()
{
}


void ScrollWindow::Scroll(int32_t scrollCount)
{
   uint16_t const newLine = (scrollLine_ + scrollCount);

   if (BufferSize() > screen_.MaxRows() - 1)
   {
      if (newLine < screen_.MaxRows() - 1)
      {
         scrollLine_ = screen_.MaxRows() - 1;
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
}

void ScrollWindow::ScrollTo(uint16_t scrollLine)
{
   if (BufferSize() > screen_.MaxRows() - 1)
   {
      scrollLine_ = scrollLine + (screen_.MaxRows() / 2);

      if (scrollLine_ < screen_.MaxRows() - 1)
      {
         scrollLine_ = screen_.MaxRows() - 1;
      }
      else if (scrollLine_ > BufferSize())
      {
         scrollLine_ = BufferSize();
      }
   }
}

bool ScrollWindow::Select(Position position, uint32_t count)
{
   // \todo prevent this from affecting things like the console window
   if (position == ScrollWindow::First)
   {
      int32_t scroll = FirstLine() + count;

      if (scroll > static_cast<int32_t>(LastLine())) 
      { 
         scroll = LastLine(); 
      }

      ScrollTo(scroll);
   }
   else if (position == ScrollWindow::Last)
   {
      int32_t scroll = LastLine() - count + 1;

      if (scroll < static_cast<int32_t>(FirstLine() + 1)) 
      { 
         scroll = FirstLine() + 1; 
      }

      ScrollTo(scroll);
   }
   else if (position == ScrollWindow::Middle)
   {
      ScrollTo(FirstLine() + ((LastLine() - FirstLine() + 1) / 2) );
   }

   return true;
}

void ScrollWindow::SetAutoScroll(bool autoScroll)
{
   autoScroll_ = autoScroll;
}

bool ScrollWindow::AutoScroll() const
{
   return autoScroll_;
}


uint32_t ScrollWindow::FirstLine() const
{
   uint16_t result = 0;

   if ((scrollLine_ - (screen_.MaxRows() - 1)) > 0)
   {
      result = (scrollLine_ - (screen_.MaxRows() - 1));
   }

   return result;
}

void ScrollWindow::ResetScroll()
{
   scrollLine_ = screen_.MaxRows();
}

uint16_t ScrollWindow::ScrollLine() const
{
   return scrollLine_;
}

void ScrollWindow::SetScrollLine(uint16_t scrollLine)
{
   scrollLine_ = scrollLine;
}
