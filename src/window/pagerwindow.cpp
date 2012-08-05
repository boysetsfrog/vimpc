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

#include "pagerwindow.hpp"

#include "colour.hpp"
#include "debug.hpp"
#include "settings.hpp"

#include <iostream>

using namespace Ui;

PagerWindow::PagerWindow(int columns, int lines) :
   Window         (0, columns, 0, 0),
   buffer_        ()
{
}

PagerWindow::~PagerWindow()
{
}


void PagerWindow::AddLine(std::string const & line)
{
   buffer_.Add(line);
}

void PagerWindow::AddLine(char const * const fmt, ...)
{
   static uint16_t const InputBufferSize = 256;
   char   buf[InputBufferSize];

   va_list args;
   va_start(args, fmt);
   vsnprintf(buf, InputBufferSize - 1, fmt, args);
   va_end(args);

   buffer_.Add(buf);
}

void PagerWindow::Print(uint32_t line) const
{
   WINDOW * const window = N_WINDOW();

   if (line == BufferSize() - 1)
   {
      if (Main::Settings::Instance().Get(Setting::ColourEnabled) == true)
      {
         wattron(N_WINDOW(), COLOR_PAIR(Colour::PagerStatus) | A_BOLD);
      }

      mvwprintw(window, line, 0, "%s", "Press ENTER to continue");

      if (Main::Settings::Instance().Get(Setting::ColourEnabled) == true)
      {
         wattroff(N_WINDOW(), COLOR_PAIR(Colour::PagerStatus) | A_BOLD);
      }
   }
   else
   {
      mvwprintw(window, line, 0, "%s", buffer_.Get(line).c_str());
   }
}

void PagerWindow::Clear()
{
   buffer_.Clear();
}

/* vim: set sw=3 ts=3: */
