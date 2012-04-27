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

#include "window/console.hpp"
#include "buffer/browse.hpp"
#include "buffer/library.hpp"
#include "buffer/list.hpp"
#include "buffer/outputs.hpp"
#include "buffer/playlist.hpp"

static Mpc::Playlist * p_buffer  = NULL;
static Mpc::Playlist * pt_buffer = NULL;
static Mpc::Playlist * t_buffer  = NULL;
static Mpc::Browse *   b_buffer  = NULL;
static Mpc::Library *  l_buffer  = NULL;
static Mpc::Lists *    i_buffer  = NULL;
static Mpc::Outputs *  o_buffer  = NULL;
static Ui::Console *   c_buffer  = NULL;

void Main::Delete()
{
   delete l_buffer;
   delete p_buffer;
   delete pt_buffer;
   delete t_buffer;
   delete b_buffer;
   delete i_buffer;
   delete o_buffer;
   delete c_buffer;
}

Mpc::Playlist & Main::Playlist()
{
   if (p_buffer == NULL)
   {
      p_buffer = new Mpc::Playlist(true);
   }
   return *p_buffer;
}

Mpc::Playlist & Main::PlaylistPasteBuffer()
{
   if (pt_buffer == NULL)
   {
      pt_buffer = new Mpc::Playlist();
   }
   return *pt_buffer;
}

Mpc::Playlist & Main::PlaylistTmp()
{
   if (t_buffer == NULL)
   {
      t_buffer = new Mpc::Playlist();
   }
   return *t_buffer;
}

Mpc::Browse & Main::Browse()
{
   if (b_buffer == NULL)
   {
      b_buffer = new Mpc::Browse();
   }
   return *b_buffer;
}

Mpc::Library & Main::Library()
{
   if (l_buffer == NULL)
   {
      l_buffer = new Mpc::Library();
   }
   return *l_buffer;
}

Mpc::Lists & Main::Lists()
{
   if (i_buffer == NULL)
   {
      i_buffer = new Mpc::Lists();
   }
   return *i_buffer;
}

Mpc::Outputs & Main::Outputs()
{
   if (o_buffer == NULL)
   {
      o_buffer = new Mpc::Outputs();
   }
   return *o_buffer;
}

Ui::Console & Main::Console()
{
   if (c_buffer == NULL)
   {
      c_buffer = new Ui::Console();
   }
   return *c_buffer;
}
/* vim: set sw=3 ts=3: */
