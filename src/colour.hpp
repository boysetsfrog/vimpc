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

#define BLUEONBLACK    (COLOR_PAIRS - 1)
#define YELLOWONBLACK  (COLOR_PAIRS - 2)
#define REDONBLACK     (COLOR_PAIRS - 3)
#define WHITEONBLACK   (COLOR_PAIRS - 4)
#define CYANONBLACK    (COLOR_PAIRS - 5)
#define GREENONBLACK   (COLOR_PAIRS - 6)
#define MAGENTAONBLACK (COLOR_PAIRS - 7)
#define WHITEONRED     (COLOR_PAIRS - 8)
#define WHITEONBLUE    (COLOR_PAIRS - 9)

#define CURRENTSONGCOLOUR    BLUEONBLACK 
#define ERRORCOLOUR          WHITEONRED 
#define SONGCOLOUR           WHITEONBLACK 
#define SONGIDCOLOUR         YELLOWONBLACK 
#define SONGMATCHCOLOUR      GREENONBLACK 
#define STATUSLINECOLOUR     WHITEONBLUE

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

            init_pair(BLUEONBLACK,    COLOR_BLUE,     -1);
            init_pair(YELLOWONBLACK,  COLOR_YELLOW,   -1);
            init_pair(REDONBLACK,     COLOR_RED,      -1);
            init_pair(WHITEONBLACK,   -1,             -1);
            init_pair(CYANONBLACK,    COLOR_CYAN,     -1);
            init_pair(GREENONBLACK,   COLOR_GREEN,    -1);
            init_pair(MAGENTAONBLACK, COLOR_MAGENTA,  -1);
            init_pair(WHITEONRED,     COLOR_WHITE,    COLOR_RED);
            init_pair(WHITEONBLUE,    COLOR_WHITE,    COLOR_BLUE);
         }
      }
   }
}

#endif
