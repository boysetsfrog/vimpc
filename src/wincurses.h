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

   wincurses.h - includes a curses header file
   */

#include "config.h"

#ifdef HAVE_NCURSESW_H
   #include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
   #include <ncurses.h>
#elif HAVE_CURSES_H
   #include <curses.h>
#else
   #include <ncurses.h>
#endif
