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

#define BACKGROUND(X) (16 + (16 * X))
#define BOLD(X) (8 + X)

using namespace Main;

Colours::Colours()
{
   Song           = COLOR_WHITE;
   SongId         = COLOR_RED;
   Directory      = COLOR_RED;
   CurrentSong    = COLOR_BLUE;
   TabWindow      = BACKGROUND(COLOR_BLUE);
   ProgressWindow = COLOR_RED;
   SongMatch      = COLOR_YELLOW;
   PartialAdd     = COLOR_CYAN;
   FullAdd        = COLOR_GREEN;
   PagerStatus    = COLOR_GREEN;

   Error          = BACKGROUND(COLOR_RED);
   StatusLine     = BACKGROUND(COLOR_BLUE);
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
         int f, b;
         for( b = -1; b < 16; ++b ) {
            for( f = 0; f < 16; ++f ) {
               if (b == -1)
                  init_pair(f, f, b);
               else
                  init_pair((b * 16) + 16 + f, f, b );
            }
         }
      }
   }

   return success;
}

/* vim: set sw=3 ts=3: */
