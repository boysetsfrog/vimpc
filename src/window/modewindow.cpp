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

   modewindow.cpp - provides a window for each input mode
   */

#include "modewindow.hpp"

#include <iostream>

using namespace Ui;

ModeWindow::ModeWindow(int columns, int lines) :
   Window         (0, columns, lines - 1, 0),
   cursorVisible_ (false),
   cursorPosition_(0),
   buffer_        ()
{
}

ModeWindow::~ModeWindow()
{
}


void ModeWindow::SetLine(std::string const & line)
{
   buffer_.Add(line);
}

void ModeWindow::SetLine(char const * const fmt, ...)
{
   static uint16_t const InputBufferSize = 256;
   char   buf[InputBufferSize];

   va_list args;
   va_start(args, fmt);
   vsnprintf(buf, InputBufferSize - 1, fmt, args);
   va_end(args);

   buffer_.Add(buf);
}

void ModeWindow::Print(uint32_t line) const
{
   line = 0;

   WINDOW * const window = N_WINDOW();

   curs_set((cursorVisible_ == true) ? 1 : 0);
   noecho();

   werase(window);

   if (buffer_.Size() > 0)
   {
      mvwprintw(window, line, 0, "%s", buffer_.Get(0).c_str());
   }

   wmove(window, line, cursorPosition_);
   wrefresh(window);
}

void ModeWindow::SetCursorPosition(uint32_t cursorPosition)
{
   cursorPosition_ = cursorPosition;
}

void ModeWindow::ShowCursor()
{
   cursorVisible_ = true;
}

void ModeWindow::HideCursor()
{
   cursorVisible_ = false;
}
/* vim: set sw=3 ts=3: */
