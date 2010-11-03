#ifndef __DBC
#define __DBC

#ifdef _DEBUG

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

namespace DBC
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
