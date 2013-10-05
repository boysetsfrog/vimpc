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

#include "player.hpp"
#include "window/window.hpp"

#include <iostream>

using namespace Ui;

Window::Window(int h, int w, int x, int y) :
   window_    (NULL)
{
   h = (h < 0) ? 0 : h;
   w = (w < 0) ? 0 : w;
   x = (x < 0) ? 0 : x;
   y = (y < 0) ? 0 : y;

   rows_ = h;
   cols_ = w;

   window_ = newwin(h, w, x, y);
}

Window::~Window()
{
   delwin(window_);
}


void Window::Move(int row, int column)
{
   mvwin(N_WINDOW(), row, column);
}

void Window::Resize(uint32_t rows, uint32_t columns)
{
   rows_ = rows;
   cols_ = columns;
   wresize(N_WINDOW(), rows, columns);
}

void Window::Erase()
{
   werase(N_WINDOW());
}

void Window::Refresh()
{
   wnoutrefresh(N_WINDOW());
}
/* vim: set sw=3 ts=3: */
