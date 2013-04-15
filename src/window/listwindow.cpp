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

   list.cpp - handling of the mpd list interface
   */

#include "listwindow.hpp"

#include <pcrecpp.h>

#include "buffers.hpp"
#include "callback.hpp"
#include "mpdclient.hpp"
#include "settings.hpp"
#include "screen.hpp"

#include "buffer/list.hpp"
#include "buffer/playlist.hpp"
#include "mode/search.hpp"
#include "window/error.hpp"
#include "window/songwindow.hpp"

using namespace Ui;

ListWindow::ListWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Lists & lists, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (settings, screen, "lists"),
   settings_        (settings),
   client_          (client),
   search_          (search),
   lists_           (lists)
{
   SetSupportsVisual(false);

   typedef Main::CallbackObject<Ui::ListWindow , Mpc::Lists::BufferType> WindowCallbackObject;
   typedef Main::CallbackObject<Mpc::Lists,      Mpc::Lists::BufferType> ListCallbackObject;

   SoftRedrawOnSetting(Setting::Playlists);

   lists_.AddCallback(Main::Buffer_Remove, new WindowCallbackObject  (*this, &Ui::ListWindow::AdjustScroll));
}

ListWindow::~ListWindow()
{
}


void ListWindow::Redraw()
{
   SoftRedraw();
}

void ListWindow::SoftRedraw()
{
   Clear();
   client_.ForEachPlaylist(lists_, static_cast<void (Mpc::Lists::*)(Mpc::List)>(&Mpc::Lists::Add));

   lists_.Sort();
   SetScrollLine(0);
   ScrollTo(0);
}

void ListWindow::Print(uint32_t line) const
{
#if 1
   uint32_t printLine = line + FirstLine();

   if (printLine < BufferSize())
   {
      WINDOW * window = N_WINDOW();
      int32_t  colour = DetermineColour(printLine);

      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattron(window, COLOR_PAIR(colour));
      }

      if (printLine == CurrentLine())
      {
         wattron(window, A_REVERSE);
      }

      mvwhline(window,  line, 0, ' ', screen_.MaxColumns());
      mvwaddstr(window, line, 1, lists_.Get(printLine).name_.c_str());

      wattroff(window, A_REVERSE);

      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattroff(window, COLOR_PAIR(colour));
      }
   }
#else
   SelectWindow::Print(line);
#endif
}

void ListWindow::Left(Ui::Player & player, uint32_t count)
{
   //player.SkipSong(Ui::Player::Previous, count);
}

void ListWindow::Right(Ui::Player & player, uint32_t count)
{
   //player.SkipSong(Ui::Player::Next, count);
}

void ListWindow::Confirm()
{
   if (lists_.Size() > CurrentLine())
   {
      client_.LoadPlaylist(lists_.Get(CurrentLine()).path_);
      client_.Play(0);
   }
}

uint32_t ListWindow::Current() const
{
   return CurrentLine();
}

int32_t ListWindow::DetermineColour(uint32_t line) const
{
   int32_t colour = settings_.colours.Song;

   if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
       (search_.HighlightSearch() == true))
   {
      pcrecpp::RE expression (".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

      if (expression.FullMatch(lists_.Get(line).name_) == true)
      {
         colour = settings_.colours.SongMatch;
      }
   }

   return colour;
}


void ListWindow::AdjustScroll(Mpc::List list)
{
   LimitCurrentSelection();
}


void ListWindow::AddLine(uint32_t line, uint32_t count, bool scroll)
{
   Main::PlaylistTmp().Clear();

   for (uint32_t i = 0; i < count; ++i)
   {
      if (i < Main::Lists().Size())
      {
         client_.ForEachPlaylistSong(Main::Lists().Get(line +  i).path_, Main::PlaylistTmp(),
                                    static_cast<void (Mpc::Playlist::*)(Mpc::Song *)>(&Mpc::Playlist::Add));
      }
   }

   uint32_t total = Main::PlaylistTmp().Size();

   if (total > 0)
   {
      Mpc::CommandList list(client_, (total > 1));

      for (uint32_t i = 0; i < total; ++i)
      {
         Main::Playlist().Add(Main::PlaylistTmp().Get(i));
         client_.Add(Main::PlaylistTmp().Get(i));
      }
   }

   if (scroll == true)
   {
      Scroll(count);
   }
}

void ListWindow::AddAllLines()
{
   AddLine(0, BufferSize(), false);
}

void ListWindow::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
   Main::PlaylistTmp().Clear();

   for (uint32_t i = 0; i < count; ++i)
   {
      if (i < Main::Lists().Size())
      {
         client_.ForEachPlaylistSong(Main::Lists().Get(line +  i).path_, Main::PlaylistTmp(),
                                    static_cast<void (Mpc::Playlist::*)(Mpc::Song *)>(&Mpc::Playlist::Add));
      }
   }

   uint32_t total = Main::PlaylistTmp().Size();

   if (total > 0)
   {
      Mpc::CommandList list(client_, (total > 1));

      for (uint32_t i = 0; i < total; ++i)
      {
         int const PlaylistIndex = Main::Playlist().Index(Main::PlaylistTmp().Get(i));

         if (PlaylistIndex >= 0)
         {
            client_.Delete(PlaylistIndex);
            Main::Playlist().Remove(PlaylistIndex, 1);
         }
      }
   }

   if (scroll == true)
   {
      Scroll(count);
   }
}

void ListWindow::DeleteAllLines()
{
   DeleteLine(0, BufferSize(), false);
}

void ListWindow::CropLine(uint32_t line, uint32_t count, bool scroll)
{
   for (unsigned int i = 0; i < count; ++i)
   {
      if (line + i < BufferSize())
      {
         client_.RemovePlaylist(Main::Lists().Get(line).path_);
         Main::Lists().Remove(line, 1);
      }
   }

   Main::Lists().Sort();
   SelectWindow::DeleteLine(line, count, scroll);
}

void ListWindow::CropAllLines()
{
   DeleteLine(CurrentLine(), BufferSize() - CurrentLine(), false);
}

void ListWindow::Edit()
{
   if (lists_.Size() > 0)
   {
      Mpc::List const playlist(lists_.Get(CurrentLine()));

      SongWindow * window = screen_.CreateSongWindow("P:" + playlist.name_);
      client_.ForEachPlaylistSong(playlist.path_, window->Buffer(), static_cast<void (Main::Buffer<Mpc::Song *>::*)(Mpc::Song *)>(&Mpc::Browse::Add));

      if (window->BufferSize() > 0)
      {
         screen_.SetActiveAndVisible(screen_.GetWindowFromName(window->Name()));
      }
      else
      {
         screen_.SetVisible(screen_.GetWindowFromName(window->Name()), false);
         ErrorString(ErrorNumber::PlaylistEmpty);
      }
   }
}


void ListWindow::Clear()
{
   ScrollTo(0);
   lists_.Clear();
}
/* vim: set sw=3 ts=3: */
