/*
   Vimpc
   Copyright (C) 2010 - 2011 Nathan Sweetman

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

#include "wincurses.h"

#define COLOR_MAX        16

#define DEFAULT          (COLOR_MAX - 1)

#define BLUEONDEFAULT    (COLOR_MAX - 2)
#define REDONDEFAULT     (COLOR_MAX - 3)
#define YELLOWONDEFAULT  (COLOR_MAX - 4)
#define CYANONDEFAULT    (COLOR_MAX - 5)
#define GREENONDEFAULT   (COLOR_MAX - 6)

#define DEFAULTONBLUE    (COLOR_MAX - 7)
#define DEFAULTONRED     (COLOR_MAX - 8)
#define DEFAULTONYELLOW  (COLOR_MAX - 9)
#define DEFAULTONCYAN    (COLOR_MAX - 10)
#define DEFAULTONGREEN   (COLOR_MAX - 11)

#define ERRORLINE        (COLOR_MAX - 12)
#define STATUSLINE       (COLOR_MAX - 13)

namespace Ui
{
   namespace Colour
   {
      static int Song         = DEFAULT;
      static int SongId       = REDONDEFAULT;
      static int Directory    = REDONDEFAULT;
      static int CurrentSong  = BLUEONDEFAULT;
      static int SongMatch    = YELLOWONDEFAULT;
      static int PartialAdd   = CYANONDEFAULT;
      static int FullAdd      = GREENONDEFAULT;
      static int PagerStatus  = GREENONDEFAULT;

      static int Error        = ERRORLINE;
      static int StatusLine   = STATUSLINE;

      static bool InitialiseColours();

      bool InitialiseColours()
      {
         static bool coloursInitialised = false;
         static bool success            = false;

         if (coloursInitialised == false)
         {
            coloursInitialised = true;

            if ((start_color() != ERR) && (use_default_colors() != ERR))
            {
                success = true;

                init_pair(DEFAULT,          -1,             -1);

                init_pair(BLUEONDEFAULT,    COLOR_BLUE,     -1);
                init_pair(YELLOWONDEFAULT,  COLOR_YELLOW,   -1);
                init_pair(REDONDEFAULT,     COLOR_RED,      -1);
                init_pair(CYANONDEFAULT,    COLOR_CYAN,     -1);
                init_pair(GREENONDEFAULT,   COLOR_GREEN,    -1);

                init_pair(DEFAULTONBLUE,    -1, COLOR_BLUE);
                init_pair(DEFAULTONRED,     -1, COLOR_RED);
                init_pair(DEFAULTONYELLOW,  -1, COLOR_YELLOW);
                init_pair(DEFAULTONCYAN,    -1, COLOR_CYAN);
                init_pair(DEFAULTONGREEN,   -1, COLOR_GREEN);

                init_pair(ERRORLINE,        -1, COLOR_RED);
                init_pair(STATUSLINE,       -1, COLOR_BLUE);
            }
         }

         return success;
      }
   }
}

#endif
/* vim: set sw=3 ts=3: */
