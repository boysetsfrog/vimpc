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

   debug.hpp - console used to display a debug print
   */

#ifndef __UI__DEBUG
#define __UI__DEBUG

#include "buffers.hpp"
#include "window/console.hpp"

#include <string>

//! Display an error window with the given error
static void Debug(std::string format, ...);

void Debug(std::string format, ...)
{
#ifdef __DEBUG_PRINTS
   char buffer[1024];
   va_list args;
   va_start(args, format);
   vsprintf(buffer, format.c_str(), args);
   Main::DebugConsole().Add(buffer);
   va_end(args);
#endif
}

#endif
/* vim: set sw=3 ts=3: */
