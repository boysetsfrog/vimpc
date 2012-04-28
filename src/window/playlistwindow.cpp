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

   playlist.cpp - handling of the mpd playlist interface
   */

#include "playlistwindow.hpp"

#include <pcrecpp.h>

#include "buffers.hpp"
#include "callback.hpp"
#include "colour.hpp"
#include "mpdclient.hpp"
#include "settings.hpp"
#include "screen.hpp"

#include "buffer/list.hpp"
#include "mode/search.hpp"
#include "window/console.hpp"

using namespace Ui;

PlaylistWindow::PlaylistWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Ui::Search const & search) :
   SongWindow       (settings, screen, client, search, "playlist"),
   settings_        (settings),
   client_          (client),
   search_          (search),
   playlist_        (Main::Playlist()),
   pasteBuffer_     (Main::PlaylistPasteBuffer())
{
   typedef Main::CallbackObject<Ui::PlaylistWindow, Mpc::Playlist::BufferType> WindowCallbackObject;
   typedef Main::CallbackObject<Mpc::Playlist,      Mpc::Playlist::BufferType> PlaylistCallbackObject;

   playlist_.AddCallback(Main::Buffer_Remove, new WindowCallbackObject  (*this,        &Ui::PlaylistWindow::AdjustScroll));
   playlist_.AddCallback(Main::Buffer_Remove, new PlaylistCallbackObject(pasteBuffer_, &Mpc::Playlist::Add));
}

PlaylistWindow::~PlaylistWindow()
{
   playlist_.Clear();
}


void PlaylistWindow::Redraw()
{
   uint16_t currentLine = CurrentLine();
   uint16_t scrollLine  = ScrollLine();

   Mpc::Song * song = NULL;

   if (currentLine < playlist_.Size())
   {
      song = playlist_.Get(currentLine);
   }

   Clear();

   client_.ForEachQueuedSong(playlist_, static_cast<void (Mpc::Playlist::*)(Mpc::Song *)>(&Mpc::Playlist::Add));

   // If we are redrawing and can keep the same scroll point do so
   // otherwise if we are redrawing due to a new playlist load etc, we need to scroll to the start
   if ((song != NULL) && (currentLine < playlist_.Size()) && (song->URI() == playlist_.Get(currentLine)->URI()))
   {
      SetScrollLine(scrollLine);
      ScrollTo(currentLine);
   }
}

void PlaylistWindow::Left(Ui::Player & player, uint32_t count)
{
   client_.Seek(-1 * count);
}

void PlaylistWindow::Right(Ui::Player & player, uint32_t count)
{
   client_.Seek(count);
}

void PlaylistWindow::Confirm()
{
   if (playlist_.Size() > CurrentLine())
   {
      Mpc::Song const * const song = playlist_.Get(CurrentLine());

      if (song != NULL)
      {
         client_.Play(static_cast<uint32_t>(CurrentLine()));
      }
   }
}

uint32_t PlaylistWindow::Current() const
{
   int32_t current = client_.GetCurrentSong();

   if (current < 0)
   {
      return CurrentLine();
   }

   return current;
}

int32_t PlaylistWindow::DetermineSongColour(uint32_t line, Mpc::Song const * const song) const
{
   int32_t colour = Colour::Song;

   if (song != NULL)
   {
      if ((client_.GetCurrentSong() > -1) && (line == static_cast<uint32_t>(client_.GetCurrentSong())))
      {
         colour = Colour::CurrentSong;
      }
      else if ((search_.LastSearchString() != "") && (settings_.HightlightSearch() == true) &&
               (search_.HighlightSearch() == true))
      {
         pcrecpp::RE expression (".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

         if (expression.FullMatch(song->FormatString(settings_.SongFormat())))
         {
            colour = Colour::SongMatch;
         }
      }
   }

   return colour;
}


void PlaylistWindow::AdjustScroll(Mpc::Song *)
{
   LimitCurrentSelection();
   ScrollTo(CurrentLine());
}

void PlaylistWindow::AddLine(uint32_t line, uint32_t count, bool scroll)
{
}

void PlaylistWindow::AddAllLines()
{
   client_.AddAllSongs();
   ScrollTo(CurrentLine());
}

void PlaylistWindow::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
   Main::PlaylistPasteBuffer().Clear();

   int64_t pos1 = CurrentSelection().first;
   int64_t pos2 = CurrentSelection().second;

   if (pos2 < pos1)
   {
      pos2 = pos1;
      pos1 = CurrentSelection().second;
   }

   if (pos1 != pos2)
   {
      count  = pos2 - pos1 + 1;
      line   = pos1;
      scroll = false;
   }

   playlist_.Remove(line, count);
   client_.Delete(line, count + line);
   ScrollTo(line);

   SelectWindow::DeleteLine(line, count, scroll);
}

void PlaylistWindow::DeleteAllLines()
{
   Main::PlaylistPasteBuffer().Clear();
   playlist_.Clear();
   client_.Clear();
}

void PlaylistWindow::Save(std::string const & name)
{
   if (Main::Lists().Index(name) == -1)
   {
      Main::Lists().Add(name);
      Main::Lists().Sort();
   }

   client_.SavePlaylist(name);
}


void PlaylistWindow::PrintId(uint32_t Id) const
{
   if (settings_.PlaylistNumbers() == true)
   {
      SongWindow::PrintId(Id);
   }
   else
   {
      SongWindow::PrintBlankId();
   }
}


void PlaylistWindow::Clear()
{
   ScrollTo(0);
   playlist_.Clear();
}
/* vim: set sw=3 ts=3: */
