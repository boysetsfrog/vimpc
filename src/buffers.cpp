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

   buffers.cpp - global access to important buffers (playlist, library, console)
   */

#include "buffers.hpp"

Mpc::Playlist & Main::Playlist()
{
   static Mpc::Playlist * buffer = new Mpc::Playlist(true);
   return *buffer;
}

Mpc::Playlist & Main::PlaylistPasteBuffer()
{
   static Mpc::Playlist * buffer = new Mpc::Playlist();
   return *buffer;
}

Mpc::Playlist & Main::PlaylistTmp()
{
   static Mpc::Playlist * buffer = new Mpc::Playlist();
   return *buffer;
}

Mpc::Browse & Main::Browse()
{
   static Mpc::Browse * buffer = new Mpc::Browse();
   return *buffer;
}

Mpc::Library & Main::Library()
{
   static Mpc::Library * buffer = new Mpc::Library();
   return *buffer;
}

Mpc::Lists & Main::Lists()
{
   static Mpc::Lists * buffer = new Mpc::Lists();
   return *buffer;
}

Mpc::Outputs & Main::Outputs()
{
   static Mpc::Outputs * buffer = new Mpc::Outputs();
   return *buffer;
}

Ui::Console & Main::Console()
{
   static Ui::Console * buffer = new Ui::Console();
   return *buffer;
}
/* vim: set sw=3 ts=3: */
