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

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

void breakpoint()
{
}

ASSERT_FUNCTION()
{
   BREAKPOINT

   int const BufferSize = 128;
   void    * buffer[BufferSize];
   int       nptrs;

   endwin();
   std::cout << "ASSERTION FAILED: " << file << " in " << function << " on line " << line << std::endl << std::endl;

#ifdef HAVE_EXECINFO_H
   nptrs = backtrace(buffer, BufferSize);
   backtrace_symbols_fd(buffer, nptrs, 2);
#endif
   exit(1);
}

#endif

int main(int argc, char** argv)
{
   bool runVimpc           = true;
   int  option             = 0;
   int  option_index       = 0;

   std::string hostname("");
   uint16_t    port(0);

   while (option != -1)
   {
      static struct option long_options[] =
      {
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
            hostname = optarg;
         }
         else if (option == 'p')
         {
            port = atoi(optarg);
         }
         else if (option == ':' || option == '?')
         {
            runVimpc  = false;
         }

         std::cout << output << std::endl;
      }
   }

   if (runVimpc == true)
   {
      setlocale(LC_ALL, "");

      Main::Vimpc vimpc;
      vimpc.Run(hostname, port);
   }

   Main::Delete();

   return 0;
}
/* vim: set sw=3 ts=3: */
