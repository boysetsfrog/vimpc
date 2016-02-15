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

   debug.cpp - console used to display a debug print
   */

#include "window/debug.hpp"

#include "buffers.hpp"
#include "compiler.hpp"

#include <stdio.h>
#include <stdarg.h>

void Debug(std::string format, ...)
{
#ifdef __DEBUG_PRINTS

   static Mutex DebugMutex;

   DebugMutex.lock();
   char buffer[1024];
   va_list args;
   va_start(args, format);
   vsnprintf(buffer, 1023, format.c_str(), args);
	buffer[1023] = '\0';
   Main::DebugConsole().Add(buffer);
   va_end(args);
   DebugMutex.unlock();
#endif
}

/* vim: set sw=3 ts=3: */
