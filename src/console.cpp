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

   console.cpp - window to accept command mode input 
   */

#include "console.hpp"

#include "screen.hpp"

#include <algorithm>

using namespace Ui;

ConsoleWindow::ConsoleWindow(Ui::Screen const & screen) :
   ScrollWindow(screen),
   buffer_     ()
{
}

ConsoleWindow::~ConsoleWindow()
{
}


void ConsoleWindow::Print(uint32_t line) const
{
   uint32_t const currentLine(FirstLine() + line);

   if (currentLine < BufferSize())
   {
      std::string const output = buffer_[currentLine].substr(0, buffer_[currentLine].find("\n"));

      mvwprintw(N_WINDOW(), line, 0, "%s", output.c_str());
   }
}


void ConsoleWindow::OutputLine(char const * const fmt, ...)
{
   static uint16_t const InputBufferSize = 256;
   char   buffer[InputBufferSize];

   va_list args;
   va_start(args, fmt);
   vsnprintf(buffer, InputBufferSize - 1, fmt, args);
   va_end(args);

   if ((AutoScroll() == true) && (BufferSize() == ScrollLine()))
   {
      wscrl(N_WINDOW(), 1);
      SetScrollLine(ScrollLine() + 1);
   }

   buffer_.insert(buffer_.end(), buffer);
}

void ConsoleWindow::Clear() 
{ 
   buffer_.clear(); 
   werase(N_WINDOW());
   ResetScroll(); 
}


size_t ConsoleWindow::BufferSize() const 
{ 
   return buffer_.size();
}
