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

   main.cpp - handle command line options and run program 
   */

#include <getopt.h>
#include <iostream>

#include "project.hpp"
#include "vimpc.hpp"

#ifdef __DEBUG_ASSERT

#include <execinfo.h>

ASSERT_FUNCTION()
{
   int const BufferSize = 128;
   void    * buffer[BufferSize];
   int       nptrs;

   endwin();
   std::cout << "ASSERTION FAILED: " << file << " in " << function << " on line " << line << std::endl << std::endl;

   nptrs = backtrace(buffer, BufferSize);
   backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
   exit(1);
}

#endif

int main(int argc, char** argv)
{
   bool runVimpc           = true;
   bool skipConfigConnects = false;
   int  option             = 0;
   int  option_index       = 0;

   while (option != -1)
   {
      static struct option long_options[] =
      {
        // TODO -h hostname
        // TODO -p port
         {"host",  required_argument, 0, 'h'},
         {"port",  required_argument, 0, 'p'},
         {"bugreport",  no_argument, 0, 'b'},
         {"url",        no_argument, 0, 'u'},
         {"version",    no_argument, 0, 'v'},
         {0, 0, 0, 0}
      };

      option = getopt_long (argc, argv, "h:p:vub", long_options, &option_index);

      if (option != -1)
      {
         std::string output; 

         if (option == 'b') 
         {
            runVimpc  = false;
            output = Main::Project::BugReport();
         }
         else if (option == 'u')
         {
            runVimpc  = false;
            output = Main::Project::URL();
         }
         else if (option == 'v')
         {
            runVimpc  = false;
            output = Main::Project::Version();
         }
         else if (option == 'h')
         {
            skipConfigConnects = true;
            setenv("MPD_HOST", optarg, 1);
         }
         else if (option == 'p')
         {
            // TODO Check that it's a valid int
            // test atoi ?
            skipConfigConnects = true;
            setenv("MPD_PORT", optarg, 1);
         }

         std::cout << output << std::endl;
      }
   }

   if (runVimpc == true)
   {
      setlocale(LC_ALL, "");

      Main::Vimpc vimpc;
      vimpc.SetSkipConfigConnects(skipConfigConnects);
      vimpc.Run();
   }

   return 0;
}
