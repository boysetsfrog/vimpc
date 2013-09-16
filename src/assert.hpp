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

#include "stdlib.h"

#ifdef __DEBUG_ASSERT
#define assert_reference(_expression)              Assert::Reference(_expression, __FILE__, __func__, __LINE__)
#define ASSERT_INFO(_expression, file, func, line) if (!(_expression)) assert_failed(file, func, line)
#define ASSERT(_expression)                        if (!(_expression)) assert_failed(__FILE__, __func__, __LINE__)
#define REQUIRE(_expression)                       ASSERT(_expression)
#define ENSURE(_expression)                        ASSERT(_expression)
#define ASSERT_FUNCTION()                          void assert_failed(const char * file, const char * function, int line)

void breakpoint();
#define BREAKPOINT breakpoint();

extern ASSERT_FUNCTION();

#define STATIC_ASSERT(exp) (static_assert_failed <(exp) >())
template<bool> struct static_assert_failed;
template<>     struct static_assert_failed<true> {};

namespace Assert
{
   // Ensure that any null pointers cause asserts rather than seg fault
   template <typename T>
   inline T & Reference(T * ptr, char const * file, char const * func, int line)
   {
      if (ptr == NULL)
      {
         ASSERT_INFO(false, file, func, line);
      }

      return *ptr;
   }

}

#else
#define assert_reference(_expression) Assert::Reference(_expression)
#define ASSERT_INFO(ignore, file, func, line) ((void) 0)
#define ASSERT(ignore)        ((void) 0)
#define REQUIRE(ignore)       ((void) 0)
#define ENSURE(ignore)        ((void) 0)
#define STATIC_ASSERT(ignore) ((void) 0)
#define BREAKPOINT

namespace Assert
{
   template <typename T>
   inline T & Reference(T * ptr)
   {
      return *ptr;
   }

}

#endif
#endif
/* vim: set sw=3 ts=3: */
