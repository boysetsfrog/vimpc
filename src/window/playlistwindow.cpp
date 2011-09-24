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
#include "mode/search.hpp"
#include "window/console.hpp"

using namespace Ui;

PlaylistWindow::PlaylistWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (screen, "playlist"),
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

void PlaylistWindow::Print(uint32_t line) const
{
   uint32_t printLine = line + FirstLine();

   if (printLine < BufferSize())
   {
      Mpc::Song const * nextSong    = playlist_.Get(printLine);
      WINDOW          * window      = N_WINDOW();
      int32_t           colour      = DetermineSongColour(line + FirstLine(), nextSong);

      if (settings_.ColourEnabled() == true)
      {
         wattron(window, COLOR_PAIR(colour));
      }

      if (printLine == CurrentLine())
      {
         wattron(window, A_REVERSE);
      }
      else if ((settings_.ColourEnabled() == true) && (colour != Colour::CurrentSong))
      {
         wattron(window, COLOR_PAIR(Colour::Song));
      }

      mvwhline(window,  line, 0, ' ', screen_.MaxColumns());
      mvwaddstr(window, line, 0, "[");

      if ((settings_.ColourEnabled() == true) && (colour != Colour::CurrentSong) && (printLine != CurrentLine()))
      {
         wattron(window, COLOR_PAIR(Colour::SongId));
      }

      wprintw(window, "%5d", FirstLine() + line + 1);

      if ((settings_.ColourEnabled() == true) && (colour != Colour::CurrentSong) && (printLine != CurrentLine()))
      {
         wattroff(window, COLOR_PAIR(Colour::SongId));
      }

      waddstr(window, "] ");

      if (settings_.ColourEnabled() == true)
      {
         wattron(window, COLOR_PAIR(colour));
      }

      std::string artist = nextSong->Artist().c_str();
      std::string title  = nextSong->Title().c_str();

      if (title == "Unknown")
      {
         title = nextSong->URI().c_str();
      }

      wprintw(window, "%s - %s", artist.c_str(), title.c_str());

      if ((settings_.ColourEnabled() == true) && (colour != Colour::CurrentSong) && (printLine != CurrentLine()))
      {
         wattron(window, COLOR_PAIR(Colour::Song));
      }

      std::string const durationString(nextSong->DurationString());
      mvwprintw(window, line, (screen_.MaxColumns() - durationString.size() - 2), "[%s]", durationString.c_str());

      if (settings_.ColourEnabled() == true)
      {
         wattroff(window, COLOR_PAIR(colour));
      }

      wattroff(window, A_REVERSE);
   }
}

void PlaylistWindow::Left(Ui::Player & player, uint32_t count)
{
   //player.SkipSong(Ui::Player::Previous, count);
}

void PlaylistWindow::Right(Ui::Player & player, uint32_t count)
{
   //player.SkipSong(Ui::Player::Next, count);
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
   return client_.GetCurrentSong();
}

int32_t PlaylistWindow::DetermineSongColour(uint32_t line, Mpc::Song const * const nextSong) const
{
   int32_t colour = Colour::Song;

   if (line == Current())
   {
      colour = Colour::CurrentSong;
   }
   else if ((search_.LastSearchString() != "") && (settings_.HightlightSearch() == true))
   {
      pcrecpp::RE expression (".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

      if (expression.FullMatch(nextSong->PlaylistDescription()) == true)
      {
         colour = Colour::SongMatch;
      }
   }

   return colour;
}


void PlaylistWindow::AdjustScroll(Mpc::Song *)
{
   currentSelection_ = LimitCurrentSelection(currentSelection_);
}


void PlaylistWindow::AddAllLines()
{
   client_.AddAllSongs();
   ScrollTo(CurrentLine());
}

void PlaylistWindow::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
   Main::PlaylistPasteBuffer().Clear();
   client_.Delete(line, count + line);
   playlist_.Remove(line, count);
   ScrollTo(line);
}

void PlaylistWindow::DeleteAllLines()
{
   Main::PlaylistPasteBuffer().Clear();
   client_.Clear();
   playlist_.Clear();
}


void PlaylistWindow::Clear()
{
   ScrollTo(0);
   playlist_.Clear();
}
