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

#include "buffers.hpp"
#include "screen.hpp"

using namespace Ui;

ConsoleWindow::ConsoleWindow(Main::Settings const & settings, Ui::Screen & screen,
                             std::string name, Console & console) :
   ScrollWindow(screen, name),
   console_    (console)
{
   console_.AddCallback(Main::Buffer_Add, [this] (Console::BufferType line) { PerformAutoScroll(line); });
}

ConsoleWindow::~ConsoleWindow()
{
}


void ConsoleWindow::PerformAutoScroll(Console::BufferType line)
{
   if ((AutoScroll() == true) && (BufferSize() - 1 <= ScrollLine()))
   {
      ScrollTo(console_.Size());
   }
}

void ConsoleWindow::Clear()
{
   console_.Clear();
   ResetScroll();
}
/* vim: set sw=3 ts=3: */
