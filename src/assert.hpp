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

   assert.hpp - provides error checking and verification 
   */

#ifndef __ASSERT
#define __ASSERT

#ifdef _VIMPC_DEBUG
#define ASSERT(_expression)  if (!(_expression)) assert_failed(__FILE__, __LINE__)
#define REQUIRE(_expression) ASSERT(_expression)
#define ENSURE(_expression)  ASSERT(_expression)
extern void assert_failed(const char * file, int line);

#define STATIC_ASSERT(exp) (static_assert_failed <(exp) >())
template<bool> struct static_assert_failed;
template<>     struct static_assert_failed<true> {};

#else
#define ASSERT(ignore)        ((void) 0)
#define REQUIRE(ignore)       ((void) 0)
#define ENSURE(ignore)        ((void) 0)
#define STATIC_ASSERT(ignore) ((void) 0)

#endif

#include "stdlib.h"
namespace Assert
{
   // Ensure that any null pointers cause asserts rather than seg fault
   template <typename T>
   inline T & Reference(T * ptr)
   {
      if (ptr == NULL)
      {
         ASSERT(false);
      }

      return *ptr;
   }
}
#endif
