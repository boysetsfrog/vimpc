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

#include "buffers.hpp"
#include "clientstate.hpp"
#include "mpdclient.hpp"
#include "regex.hpp"
#include "settings.hpp"
#include "screen.hpp"

#include "buffer/list.hpp"
#include "mode/search.hpp"
#include "window/console.hpp"
#include "window/debug.hpp"

using namespace Ui;

PlaylistWindow::PlaylistWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Playlist & playlist, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search) :
   SongWindow       (settings, screen, client, clientState, search, "playlist"),
   settings_        (settings),
   client_          (client),
   clientState_     (clientState),
   search_          (search),
   playlist_        (playlist),
   pasteBuffer_     (Main::PlaylistPasteBuffer())
{
   playlist_.AddCallback(Main::Buffer_Remove, [this] (Mpc::Playlist::BufferType line) { AdjustScroll(line); });
   playlist_.AddCallback(Main::Buffer_Remove, [this] (Mpc::Playlist::BufferType line) { this->pasteBuffer_.Add(line); });
}

PlaylistWindow::~PlaylistWindow()
{
   playlist_.Clear();
}


void PlaylistWindow::Redraw()
{
   ScrollTo(CurrentLine());
}

void PlaylistWindow::Left(Ui::Player & player, uint32_t count)
{
   client_.Seek(-1 * count);
}

void PlaylistWindow::Right(Ui::Player & player, uint32_t count)
{
   client_.Seek(count);
}

uint32_t PlaylistWindow::Current() const
{
   int32_t current = clientState_.GetCurrentSongPos();

   if (current < 0)
   {
      return CurrentLine();
   }

   return current;
}

int32_t PlaylistWindow::DetermineColour(uint32_t line) const
{
   int32_t colour = settings_.colours.Song;

   if (line + FirstLine() < Buffer().Size())
   {
      Mpc::Song const * const song = Buffer().Get(line + FirstLine());

      if (song != NULL)
      {
         if ((clientState_.GetCurrentSongPos() > -1) && ((line + FirstLine()) == static_cast<uint32_t>(clientState_.GetCurrentSongPos())))
         {
            colour = settings_.colours.CurrentSong;
         }
         else if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
                  (search_.HighlightSearch() == true))
         {
            Regex::RE expression (".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

            if (expression.CompleteMatch(song->FormatString(settings_.Get(Setting::SongFormat))))
            {
               colour = settings_.colours.SongMatch;
            }
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
   if (Buffer().Size() > 0)
   {
      Main::PlaylistPasteBuffer().Clear();
   }

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

   client_.Delete(line, count + line);
   ScrollTo(line);

   SelectWindow::DeleteLine(line, count, scroll);
}

void PlaylistWindow::DeleteAllLines()
{
   if (Buffer().Size() > 0)
   {
      Main::PlaylistPasteBuffer().Clear();
   }

   playlist_.Clear();
   client_.Clear();
}

void PlaylistWindow::Save(std::string const & name)
{
   client_.SavePlaylist(name);
}


void PlaylistWindow::PrintId(uint32_t Id) const
{
   if (settings_.Get(Setting::PlaylistNumbers) == true)
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
