#include <iostream>
#include <ncurses.h>
#include <stdlib.h>

#include "vimpc.hpp"

#ifdef _DEBUG

#include <execinfo.h>

extern void assert_failed(const char * file, int line)
{
   int const BufferSize = 128;
   void    * buffer[BufferSize];
   int       nptrs;

   endwin();
   std::cout << "DBC ASSERTION FAILED: " << file << " on line " << line << std::endl << std::endl;

   nptrs = backtrace(buffer, BufferSize);
   backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
   exit(1);
}

#endif

int main()
{
	Main::Vimpc vimpc;

	vimpc.Run();
   return 0;
}
