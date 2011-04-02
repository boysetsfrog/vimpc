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

   playlist.cpp - handling of the mpd playlist interface 
   */

#include "playlistwindow.hpp"

#include "buffers.hpp"
#include "callback.hpp"
#include "colour.hpp"
#include "mpdclient.hpp"
#include "settings.hpp"
#include "screen.hpp"
#include "mode/search.hpp"
#include "window/console.hpp"

#include <boost/regex.hpp>

using namespace Ui;

PlaylistWindow::PlaylistWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (screen),
   settings_        (settings),
   client_          (client),
   search_          (search),
   playlist_        (Main::Playlist()),
   pasteBuffer_     (Main::PlaylistPasteBuffer())
{
   typedef Main::CallbackObject<Mpc::Playlist, Mpc::Playlist::BufferType> CallbackObject;
   playlist_.AddCallback(Main::Buffer_Remove, new CallbackObject(pasteBuffer_, &Mpc::Playlist::Add));
}

PlaylistWindow::~PlaylistWindow()
{
}


void PlaylistWindow::Redraw()
{
   uint16_t currentLine = CurrentLine();
   uint16_t scrollLine  = ScrollLine();

   Clear();

   client_.ForEachQueuedSong(playlist_, static_cast<void (Mpc::Playlist::*)(Mpc::Song *)>(&Mpc::Playlist::Add));

   SetScrollLine(scrollLine);
   ScrollTo(currentLine);
}

void PlaylistWindow::Print(uint32_t line) const
{
   uint32_t printLine = line + FirstLine();

   if (printLine < BufferSize())
   {
      Mpc::Song const * nextSong    = playlist_.Get(printLine);
      WINDOW          * window      = N_WINDOW();
      int32_t           colour      = DetermineSongColour(line + FirstLine(), nextSong);
      
      wattron(window, COLOR_PAIR(colour) | A_BOLD);

      if (printLine == CurrentLine())
      {
         wattron(window, A_REVERSE);
      }
      else if (colour != CURRENTSONGCOLOUR)
      {
         wattron(window, COLOR_PAIR(SONGCOLOUR));
      }

      mvwhline(window,  line, 0, ' ', screen_.MaxColumns());
      mvwaddstr(window, line, 0, "[");

      if ((colour != CURRENTSONGCOLOUR) && (printLine != CurrentLine()))
      {
         wattron(window, COLOR_PAIR(SONGIDCOLOUR));
      }

      wprintw(window, "%5d", FirstLine() + line + 1);

      if ((colour != CURRENTSONGCOLOUR) && (printLine != CurrentLine()))
      {
         wattroff(window, COLOR_PAIR(SONGIDCOLOUR));
      }

      waddstr(window, "] ");

      if (colour != CURRENTSONGCOLOUR)
      {
         wattroff(window, A_BOLD);
      }

      wattron(window, COLOR_PAIR(colour));

      std::string artist = nextSong->Artist().c_str();
      std::string title  = nextSong->Title().c_str();

      if (title == "Unknown")
      {
         title = nextSong->URI().c_str();
      }

      wprintw(window, "%s - %s", artist.c_str(), title.c_str());

      if ((colour != CURRENTSONGCOLOUR) && (printLine != CurrentLine()))
      {
         wattron(window, COLOR_PAIR(SONGCOLOUR));
      }

      std::string const durationString(nextSong->DurationString());
      mvwprintw(window, line, (screen_.MaxColumns() - durationString.size() - 2), "[%s]", durationString.c_str());

      wattroff(window, A_BOLD | A_REVERSE | COLOR_PAIR(colour));
      wredrawln(window, line, 1);
   }
}

void PlaylistWindow::Left(Ui::Player & player, uint32_t count)
{
   player.SkipSong(Ui::Player::Previous, count);
}

void PlaylistWindow::Right(Ui::Player & player, uint32_t count)
{
   player.SkipSong(Ui::Player::Next, count);
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
   int32_t colour = SONGCOLOUR;

   if (line == Current())
   {
      colour = CURRENTSONGCOLOUR;
   }
   else if ((search_.LastSearchString() != "") && (settings_.HightlightSearch() == true))
   {
      boost::regex expression(".*" + search_.LastSearchString() + ".*");

      if (boost::regex_match(nextSong->PlaylistDescription(), expression))
      {
         colour = SONGMATCHCOLOUR;
      } 
   }

   return colour;
}

void PlaylistWindow::Clear()
{
   ScrollTo(0);
   playlist_.Clear();
}
