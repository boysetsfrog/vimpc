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

   selectwindow.cpp - window that is scrollable and has selectable elements 
   */

#include "selectwindow.hpp"

#include "screen.hpp"

using namespace Ui;

SelectWindow::SelectWindow(Ui::Screen const & screen) :
   ScrollWindow     (screen),
   currentSelection_(0)
{
}

SelectWindow::~SelectWindow()
{
}

void SelectWindow::Scroll(int32_t scrollCount)
{
   currentSelection_ += scrollCount;
   currentSelection_  = LimitCurrentSelection(currentSelection_);

   if ((currentSelection_ >= scrollLine_) || (currentSelection_ < scrollLine_ - (screen_.MaxRows() - 1)))
   {   
      ScrollWindow::Scroll(scrollCount);
   }
}

void SelectWindow::ScrollTo(uint16_t scrollLine)
{
   int64_t oldSelection = currentSelection_;
   currentSelection_    = (static_cast<int64_t>(scrollLine - 1));
   currentSelection_    = LimitCurrentSelection(currentSelection_);

   if (scrollLine_ > BufferSize())
   {
      scrollLine_ = BufferSize();
   }

   if ((currentSelection_ == scrollLine_) && (currentSelection_ - oldSelection == 1))
   {
      ScrollWindow::Scroll(1);
   }
   else if ((currentSelection_ == scrollLine_ - screen_.MaxRows()) && (currentSelection_ - oldSelection == -1))
   {
      ScrollWindow::Scroll(-1);
   }
   else if ((currentSelection_ >= scrollLine_) || (currentSelection_ < scrollLine_ - (screen_.MaxRows() - 1)))
   {   
      ScrollWindow::ScrollTo(scrollLine);
   }
}

uint16_t SelectWindow::CurrentLine() const
{ 
   return currentSelection_; 
}

int64_t SelectWindow::LimitCurrentSelection(int64_t currentSelection) const
{
   if (currentSelection < 0)
   {
      currentSelection = 0;
   }
   else if (currentSelection_ >= BufferSize())
   {
      currentSelection = BufferSize() - 1;
   }

   return currentSelection;
}
