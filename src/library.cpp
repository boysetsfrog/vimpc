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

   library.cpp - handling of the mpd music library 
   */

#include "library.hpp"

#include "screen.hpp"

using namespace Ui;

LibraryWindow::LibraryWindow(Ui::Screen const & screen) :
   ScrollWindow     (screen)
{
   buffer_.push_back("Library");
}

LibraryWindow::~LibraryWindow()
{
}


void LibraryWindow::Redraw()
{
   Clear();
}

void LibraryWindow::Clear()
{
   buffer_.clear();
}

void LibraryWindow::Print(uint32_t line) const
{
   static std::string const BlankLine(screen_.MaxColumns(), ' ');

   WINDOW * window = N_WINDOW();

   if (line < buffer_.size())
   {
      mvwprintw(window, 0, 0, "%s", buffer_.at(line).c_str());
   }
}

void LibraryWindow::Confirm() const
{
}

void LibraryWindow::Scroll(int32_t scrollCount)
{
   ScrollWindow::Scroll(scrollCount);
}

void LibraryWindow::ScrollTo(uint16_t scrollLine)
{
   ScrollWindow::ScrollTo(scrollLine);
}
