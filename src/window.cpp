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

#include "attributes.hpp"
#include "player.hpp"
#include "window.hpp"

#include <iostream>

using namespace Ui;

Window::Window(int h, int w, int x, int y) :
   window_    (newwin(h, w, x, y))
{
}

Window::~Window()
{
   delwin(window_);
}

void Window::Left(UNUSED Ui::Player & player, UNUSED uint32_t count)
{

}

void Window::Right(UNUSED Ui::Player & player, UNUSED uint32_t count)
{

}

void Window::Confirm()
{
}

void Window::Redraw()
{

}

void Window::Erase()
{
   werase(N_WINDOW());
}

void Window::Refresh()
{
   wnoutrefresh(N_WINDOW());
}
