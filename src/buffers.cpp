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
#include "vimpc.hpp"

#include "buffer/browse.hpp"
#include "buffer/library.hpp"
#include "buffer/directory.hpp"
#include "buffer/list.hpp"
#include "buffer/outputs.hpp"
#include "buffer/playlist.hpp"
#include "window/console.hpp"

static Mpc::Playlist *  p_buffer    = NULL;
static Mpc::Playlist *  pt_buffer   = NULL;
static Mpc::Playlist *  t_buffer    = NULL;
static Mpc::Browse *    b_buffer    = NULL;
static Mpc::Library *   l_buffer    = NULL;
static Mpc::Directory * dir_buffer  = NULL;
static Mpc::Lists *     f_buffer    = NULL;
static Mpc::Lists *     m_buffer    = NULL;
static Mpc::Lists *     i_buffer    = NULL;
static Mpc::Outputs *   o_buffer    = NULL;
static Ui::Console *    c_buffer    = NULL;
static Ui::Console *    d_buffer    = NULL;
static Ui::Console *    x_buffer    = NULL;
static Main::Lyrics *   y_buffer    = NULL;

void Main::Delete()
{
   delete l_buffer;
   delete dir_buffer;
   delete p_buffer;
   delete pt_buffer;
   delete t_buffer;
   delete b_buffer;
   delete f_buffer;
   delete m_buffer;
   delete i_buffer;
   delete o_buffer;
   delete c_buffer;
   delete d_buffer;
   delete x_buffer;
   delete y_buffer;
}

Mpc::Playlist & Main::Playlist()
{
   if (p_buffer == NULL)
   {
      p_buffer = new Mpc::Playlist(true);
      Main::Vimpc::EventHandler(Event::ClearDatabase, [] (EventData const & Data)
         { Main::Playlist().Clear(); });

      //
      Main::Vimpc::EventHandler(Event::PlaylistAdd, [] (EventData const & Data)
         {
            Mpc::Song * song = (Data.song != NULL) ? Data.song : Main::Library().Song(Data.uri);

            if (song == NULL)
            {
               song = new Mpc::Song();
               song->SetURI(Data.uri.c_str());
            }

            if (Data.pos1 == -1)
            {
               Main::Playlist().Add(song);
            }
            else
            {
               Main::Playlist().Add(song, Data.pos1);
            }
         });

      Main::Vimpc::EventHandler(Event::PlaylistQueueReplace, [] (EventData const & Data)
         {
            for (auto pair : Data.posuri)
            {
               Mpc::Song * song = (pair.second.first != NULL) ? pair.second.first : Main::Library().Song(pair.second.second);

               if (song == NULL)
               {
                  song = new Mpc::Song();
                  song->SetURI(pair.second.second.c_str());
               }

               Main::Playlist().Replace(pair.first, song);
            }

            Main::Playlist().Crop(Data.count);
         });

   }
   return *p_buffer;
}

Mpc::Playlist & Main::PlaylistPasteBuffer()
{
   if (pt_buffer == NULL)
   {
      pt_buffer = new Mpc::Playlist();
      Main::Vimpc::EventHandler(Event::ClearDatabase, [] (EventData const & Data)
         { Main::PlaylistPasteBuffer().Clear(); });
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
      Main::Vimpc::EventHandler(Event::ClearDatabase, [] (EventData const & Data)
         { Main::Library().Clear(); });
      Main::Vimpc::EventHandler(Event::DatabaseSong,  [] (EventData const & Data)
         { Main::Library().Add(Data.song); });
   }
   return *l_buffer;
}

Mpc::Directory & Main::Directory()
{
   if (dir_buffer == NULL)
   {
      dir_buffer = new Mpc::Directory();
      Main::Vimpc::EventHandler(Event::ClearDatabase, [] (EventData const & Data)
         { Main::Directory().Clear(true); });
      Main::Vimpc::EventHandler(Event::DatabaseSong,  [] (EventData const & Data)
         { Main::Directory().Add(Data.song); });
      Main::Vimpc::EventHandler(Event::DatabasePath, [] (EventData const & Data)
         { Main::Directory().Add(Data.uri); });
      Main::Vimpc::EventHandler(Event::DatabaseListFile, [] (EventData const & Data)
         { Mpc::List const list(Data.uri, Data.name); Main::Directory().AddPlaylist(list); });
   }
   return *dir_buffer;
}

Mpc::Lists & Main::FileLists()
{
   if (f_buffer == NULL)
   {
      f_buffer = new Mpc::Lists();
      Main::Vimpc::EventHandler(Event::ClearDatabase, [] (EventData const & Data)
         { Main::FileLists().Clear(); });
      Main::Vimpc::EventHandler(Event::DatabaseListFile, [] (EventData const & Data)
         { Mpc::List const list(Data.uri, Data.name); Main::FileLists().Add(list); });
   }
   return *f_buffer;
}

Mpc::Lists & Main::MpdLists()
{
   if (m_buffer == NULL)
   {
      m_buffer = new Mpc::Lists();
      Main::Vimpc::EventHandler(Event::ClearDatabase, [] (EventData const & Data)
         { Main::MpdLists().Clear(); });
      Main::Vimpc::EventHandler(Event::DatabaseList, [] (EventData const & Data)
         { Mpc::List const list(Data.name); Main::MpdLists().Add(list); });
      Main::Vimpc::EventHandler(Event::NewPlaylist, [] (EventData const & Data)
         {
            if (Main::MpdLists().Index(Mpc::List(Data.name)) == -1)
            {
               Main::MpdLists().Add(Data.name);
               Main::MpdLists().Sort();
            }
         });
   }
   return *m_buffer;
}

Mpc::Lists & Main::AllLists()
{
   if (i_buffer == NULL)
   {
      i_buffer = new Mpc::Lists();
      Main::Vimpc::EventHandler(Event::ClearDatabase, [] (EventData const & Data)
         { Main::AllLists().Clear(); });
      Main::Vimpc::EventHandler(Event::DatabaseListFile, [] (EventData const & Data)
         { Mpc::List const list(Data.uri, Data.name); Main::AllLists().Add(list); });
      Main::Vimpc::EventHandler(Event::DatabaseList, [] (EventData const & Data)
         { Mpc::List const list(Data.name); Main::AllLists().Add(list); });
      Main::Vimpc::EventHandler(Event::NewPlaylist, [] (EventData const & Data)
         {
            if (Main::AllLists().Index(Mpc::List(Data.name)) == -1)
            {
               Main::AllLists().Add(Data.name);
               Main::AllLists().Sort();
            }
         });
   }
   return *i_buffer;
}

Mpc::Outputs & Main::Outputs()
{
   if (o_buffer == NULL)
   {
      o_buffer = new Mpc::Outputs();
      Main::Vimpc::EventHandler(Event::Output, [] (EventData const & Data)
         { Main::Outputs().Add(Data.output); });
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

Ui::Console & Main::DebugConsole()
{
   if (d_buffer == NULL)
   {
      d_buffer = new Ui::Console();
   }
   return *d_buffer;
}

Ui::Console & Main::TestConsole()
{
   if (x_buffer == NULL)
   {
      x_buffer = new Ui::Console();
      Main::Vimpc::EventHandler(Event::TestResult, [] (EventData const & Data)
         { Main::TestConsole().Add(Data.name); });
   }
   return *x_buffer;
}

Main::Lyrics & Main::LyricsBuffer()
{
   if (y_buffer == NULL)
   {
      y_buffer = new Main::Lyrics();
   }
   return *y_buffer;
}
/* vim: set sw=3 ts=3: */
