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

   browsewindow.hpp - handling of the mpd library but with a playlist interface
   */

#include "browsewindow.hpp"

#include <pcrecpp.h>

#include "buffers.hpp"
#include "callback.hpp"
#include "colour.hpp"
#include "mpdclient.hpp"
#include "settings.hpp"
#include "screen.hpp"
#include "buffer/library.hpp"
#include "mode/search.hpp"
#include "window/console.hpp"

using namespace Ui;

BrowseWindow::BrowseWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (screen),
   settings_        (settings),
   client_          (client),
   search_          (search),
   browse_          (Main::Browse())
{
}

BrowseWindow::~BrowseWindow()
{
}


void BrowseWindow::Redraw()
{
   uint16_t currentLine = CurrentLine();
   uint16_t scrollLine  = ScrollLine();

   Clear();

   Main::CallbackObject<Ui::BrowseWindow, Mpc::Song * > callback(*this, &Ui::BrowseWindow::Add);

   // Do not need to sort as this ensures it will be sorted in the same order as the library
   Main::Library().ForEachSong(&callback);

   SetScrollLine(scrollLine);
   ScrollTo(currentLine);
}


void BrowseWindow::Add(Mpc::Song * song)
{
   if (song != NULL)
   {
      browse_.Add(song);
   }
}

void BrowseWindow::Print(uint32_t line) const
{
   uint32_t printLine = line + FirstLine();

   if (printLine < BufferSize())
   {
      Mpc::Song const * nextSong    = browse_.Get(printLine);
      WINDOW          * window      = N_WINDOW();
      int32_t           colour      = DetermineSongColour(nextSong);

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

void BrowseWindow::Left(Ui::Player & player, uint32_t count)
{
   player.SkipSong(Ui::Player::Previous, count);
}

void BrowseWindow::Right(Ui::Player & player, uint32_t count)
{
   player.SkipSong(Ui::Player::Next, count);
}

void BrowseWindow::Confirm()
{
   if (browse_.Size() > CurrentLine())
   {
      browse_.AddToPlaylist(client_, CurrentLine());

      if (browse_.Get(CurrentLine()) != NULL)
      {
         client_.Play(static_cast<uint32_t>(Main::Playlist().Size() - 1));
      }
   }
}

uint32_t BrowseWindow::Current() const
{
   uint32_t current       = 0;
   int32_t  currentSongId = client_.GetCurrentSong();

   if ((currentSongId >= 0) && (currentSongId < static_cast<int32_t>(Main::Playlist().Size())))
   {
      current = Main::Browse().Index(Main::Playlist().Get(currentSongId));
   }

   return current;
}

uint32_t BrowseWindow::Playlist(int Offset) const
{
   //! \todo not sure how i am going to do this
   // but it sure be used to navigate throw the browse window
   // skipping forward and backwards to songs that are in the playlist only
   return 0;
}

int32_t BrowseWindow::DetermineSongColour(Mpc::Song const * const nextSong) const
{
   int32_t colour = Colour::Song;

   if ((nextSong->URI() == client_.GetCurrentSongURI()))
   {
      colour = Colour::CurrentSong;
   }
   else if (client_.SongIsInQueue(*nextSong))
   {
      colour = Colour::FullAdd;
   }
   else if ((search_.LastSearchString() != "") && (settings_.HightlightSearch() == true))
   {
      pcrecpp::RE expression(".*" + search_.LastSearchString() + ".*");

      if (expression.FullMatch(nextSong->PlaylistDescription()))
      {
         colour = Colour::SongMatch;
      }
   }

   return colour;
}

void BrowseWindow::Clear()
{
   ScrollTo(0);
   browse_.Clear();
}
