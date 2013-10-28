/*
   Vimpc
   Copyright (C) 2010 - 2013 Nathan Sweetman

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

   vimpc.hpp - handles mode changes and input processing
   */

#ifndef __MAIN__TEST
#define __MAIN__TEST

#include "config.h"

#ifdef TEST_ENABLED
#define protected public
#endif

namespace Ui
{
   class Command;
   class Screen;
}

namespace Mpc
{
   class Client;
   class ClientState;
}

namespace Main
{
   class Vimpc;

   class Tester
   {
      protected:
         Tester() { }
         ~Tester() { }

      public:
         static Tester & Instance()
         {
            static Tester tester;
            return tester;
         }

      #ifdef TEST_ENABLED
         Main::Vimpc * Vimpc;
         Ui::Screen *  Screen;
         Ui::Command * Command;
         Mpc::Client * Client;
         Mpc::ClientState * ClientState;
      #endif
   };
}

#endif
/* vim: set sw=3 ts=3: */
