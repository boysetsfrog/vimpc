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

#ifndef __MAIN__COLOURS
#define __MAIN__COLOURS

#include "wincurses.h"

namespace Main
{
   class Colours
   {
      public:
      typedef enum {
         NONE,
         STATUSLINE,
         ERRORLINE,
         DEFAULT_ON_BLACK,
         DEFAULT_ON_WHITE,
         DEFAULT_ON_RED,
         DEFAULT_ON_GREEN,
         DEFAULT_ON_YELLOW,
         DEFAULT_ON_BLUE,
         DEFAULT_ON_MAGENTA,
         DEFAULT_ON_CYAN,
         BLACK_ON_DEFAULT,
         WHITE_ON_DEFAULT,
         RED_ON_DEFAULT,
         GREEN_ON_DEFAULT,
         YELLOW_ON_DEFAULT,
         BLUE_ON_DEFAULT,
         MAGENTA_ON_DEFAULT,
         CYAN_ON_DEFAULT,
         DEFAULT
      } ColourPairs;

      int Song;
      int SongId;
      int Directory;
      int CurrentSong;
      int TabWindow;
      int ProgressWindow;
      int SongMatch;
      int PartialAdd;
      int FullAdd;
      int PagerStatus;

      int Error;
      int StatusLine;

      Colours();
      static bool InitialiseColours();
   };
}

#endif
/* vim: set sw=3 ts=3: */
