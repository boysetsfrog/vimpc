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

   colours.hpp - provides colours for ncurses windows
   */

#include "colours.hpp"

using namespace Main;

Colours::Colours()
{

   Song         = DEFAULT;
   SongId       = RED_ON_DEFAULT;
   Directory    = RED_ON_DEFAULT;
   CurrentSong  = BLUE_ON_DEFAULT;
   TabWindow    = DEFAULT_ON_BLUE;
   ProgressWindow = RED_ON_DEFAULT;
   SongMatch    = YELLOW_ON_DEFAULT;
   PartialAdd   = CYAN_ON_DEFAULT;
   FullAdd      = GREEN_ON_DEFAULT;
   PagerStatus  = GREEN_ON_DEFAULT;

   Error        = ERRORLINE;
   StatusLine   = STATUSLINE;
}

bool Colours::InitialiseColours()
{
   static bool coloursInitialised = false;
   static bool success            = false;

   if (coloursInitialised == false)
   {
      coloursInitialised = true;

      if ((start_color() != ERR) && (use_default_colors() != ERR))
      {
         success = true;

         init_pair(DEFAULT,            -1,             -1);

         init_pair(CYAN_ON_DEFAULT,    COLOR_CYAN,     -1);
         init_pair(MAGENTA_ON_DEFAULT, COLOR_MAGENTA,  -1);
         init_pair(BLUE_ON_DEFAULT,    COLOR_BLUE,     -1);
         init_pair(YELLOW_ON_DEFAULT,  COLOR_YELLOW,   -1);
         init_pair(GREEN_ON_DEFAULT,   COLOR_GREEN,    -1);
         init_pair(RED_ON_DEFAULT,     COLOR_RED,      -1);
         init_pair(WHITE_ON_DEFAULT,   COLOR_WHITE,    -1);

         init_pair(DEFAULT_ON_CYAN,    -1, COLOR_CYAN);
         init_pair(DEFAULT_ON_MAGENTA, -1, COLOR_MAGENTA);
         init_pair(DEFAULT_ON_BLUE,    -1, COLOR_BLUE);
         init_pair(DEFAULT_ON_YELLOW,  -1, COLOR_YELLOW);
         init_pair(DEFAULT_ON_GREEN,   -1, COLOR_GREEN);
         init_pair(DEFAULT_ON_RED,     -1, COLOR_RED);
         init_pair(DEFAULT_ON_BLACK,   -1, COLOR_BLACK);
         init_pair(DEFAULT_ON_WHITE,   -1, COLOR_WHITE);

         init_pair(ERRORLINE,          -1, COLOR_RED);
         init_pair(STATUSLINE,         -1, COLOR_BLUE);
      }
   }

   return success;
}

/* vim: set sw=3 ts=3: */
