#include <getopt.h>
#include <iostream>
#include <ncurses.h>
#include <sstream>
#include <stdlib.h>

#include "vimpc.hpp"
#include "config.h"

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

namespace Project
{
   static std::string const & BugReport()
   {
      static std::string bugReport(PACKAGE_BUGREPORT);
      return bugReport;
   }
   
   static std::string const & URL()
   {
      static std::string url(PACKAGE_URL);
      return url;
   }

   static std::string const & Version()
   {
      static std::ostringstream versionBuffer;
      versionBuffer << PACKAGE_STRING  
#ifdef PACKAGE_SVN_REVISION
                    << " " << PACKAGE_SVN_REVISION 
#endif
                    << std::endl;
      static std::string const VersionString(versionBuffer.str());

      return VersionString;
   }

}

int main(int argc, char** argv)
{
   bool runVimpc     = true;
   int  option       = 0;
   int  option_index = 0;

   while (option != -1)
   {
      static struct option long_options[] =
      {
         {"version",    no_argument, 0, 'v'},
         {"url",        no_argument, 0, 'u'},
         {"bugreport",  no_argument, 0, 'b'},
         {0, 0, 0, 0}
      };

      option = getopt_long (argc, argv, "vub", long_options, &option_index);

      if (option != -1)
      {
         std::string output; 

         runVimpc  = false;

         if (option == 'b') 
         {
            output = Project::BugReport();
         }
         else if (option == 'u')
         {
            output = Project::URL();
         }
         else if (option == 'v')
         {
            output = Project::Version();
         }

         std::cout << output << std::endl;
      }
   }

   if (runVimpc == true)
   {
      Main::Vimpc vimpc;
      vimpc.Run();
   }

   return 0;
}
