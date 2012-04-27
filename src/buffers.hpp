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

   buffers.hpp - global access to important buffers (playlist, library, console)
   */

#ifndef __MAIN__BUFFERS
#define __MAIN__BUFFERS

#include <iostream>

namespace Mpc
{
   class Playlist;
   class Browse;
   class Library;
   class Lists;
   class Outputs;
}

namespace Ui
{
   class Console;
}

namespace Main
{
   void Delete();

   Mpc::Playlist & Playlist();
   Mpc::Playlist & PlaylistPasteBuffer();
   Mpc::Playlist & PlaylistTmp();
   Mpc::Browse   & Browse();
   Mpc::Library  & Library();
   Mpc::Lists    & Lists();
   Mpc::Outputs  & Outputs();
   Ui::Console   & Console();
}

#endif
/* vim: set sw=3 ts=3: */
