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

   attributes.hpp - defines gcc attribute macros 
   */

#ifndef __ATTRIBUTES
#define __ATTRIBUTES

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#define FUNCTION_IS_NOT_USED __attribute__ ((unused))
#else
#define UNUSED
#define FUNCTION_IS_NOT_USED
#endif

#endif
