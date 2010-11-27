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

   colour.hpp - provides colours for ncurses windows 
   */

#ifndef __UI__COLOUR
#define __UI__COLOUR

#include <ncursesw/ncurses.h>

#ifdef __GNUC__
#ifndef FUNCION_IS_NOT_USED
#define FUNCTION_IS_NOT_USED __attribute__ ((unused))
#endif
#else
#ifndef FUNCION_IS_NOT_USED
#define FUNCTION_IS_NOT_USED
#endif
#endif

#define DEFAULT          (COLOR_PAIRS - 1)
#define BLUEONDEFAULT    (COLOR_PAIRS - 2)
#define YELLOWONDEFAULT  (COLOR_PAIRS - 3)
#define REDONDEFAULT     (COLOR_PAIRS - 4)
#define CYANONDEFAULT    (COLOR_PAIRS - 5)
#define GREENONDEFAULT   (COLOR_PAIRS - 6)
#define MAGENTAONDEFAULT (COLOR_PAIRS - 7)
#define DEFAULTONRED     (COLOR_PAIRS - 8)
#define DEFAULTONBLUE    (COLOR_PAIRS - 9)

#define CURRENTSONGCOLOUR    BLUEONDEFAULT 
#define ERRORCOLOUR          DEFAULTONRED 
#define SONGCOLOUR           DEFAULT 
#define SONGIDCOLOUR         REDONDEFAULT 
#define SONGMATCHCOLOUR      GREENONDEFAULT 
#define STATUSLINECOLOUR     DEFAULTONBLUE

namespace Ui
{
   namespace Colour
   {
      static void InitialiseColours() FUNCTION_IS_NOT_USED;

      void InitialiseColours()
      {
         static bool coloursInitialised = false;

         if (coloursInitialised == false)
         {
            coloursInitialised = true;

            start_color();
            use_default_colors();

            init_pair(DEFAULT,          -1,             -1);
            init_pair(BLUEONDEFAULT,    COLOR_BLUE,     -1);
            init_pair(YELLOWONDEFAULT,  COLOR_YELLOW,   -1);
            init_pair(REDONDEFAULT,     COLOR_RED,      -1);
            init_pair(CYANONDEFAULT,    COLOR_CYAN,     -1);
            init_pair(GREENONDEFAULT,   COLOR_GREEN,    -1);
            init_pair(MAGENTAONDEFAULT, COLOR_MAGENTA,  -1);

            init_pair(DEFAULTONRED,     -1, COLOR_RED);
            init_pair(DEFAULTONBLUE,    -1, COLOR_BLUE);
         }
      }
   }
}

#endif
