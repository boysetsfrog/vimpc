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
#include "colour.hpp"
#include "mpdclient.hpp"
#include "settings.hpp"
#include "screen.hpp"
#include "mode/search.hpp"
#include "window/console.hpp"

using namespace Ui;

ListWindow::ListWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (screen),
   settings_        (settings),
   client_          (client),
   search_          (search),
   lists_           (Main::Lists())
{
   typedef Main::CallbackObject<Ui::ListWindow , Mpc::Lists::BufferType> WindowCallbackObject;
   typedef Main::CallbackObject<Mpc::Lists,      Mpc::Lists::BufferType> ListCallbackObject;

   lists_.AddCallback(Main::Buffer_Remove, new WindowCallbackObject  (*this, &Ui::ListWindow::AdjustScroll));
}

ListWindow::~ListWindow()
{
}


void ListWindow::Redraw()
{
   Clear();
   client_.ForEachPlaylist(lists_, static_cast<void (Mpc::Lists::*)(std::string)>(&Mpc::Lists::Add));

   SetScrollLine(0);
   ScrollTo(0);
}

void ListWindow::Print(uint32_t line) const
{
   uint32_t printLine = line + FirstLine();

   if (printLine < BufferSize())
   {
      WINDOW * window = N_WINDOW();

      if (printLine == CurrentLine())
      {
         wattron(window, A_REVERSE);
      }

      wattron(window, A_BOLD);

      mvwhline(window,  line, 0, ' ', screen_.MaxColumns());
      mvwaddstr(window, line, 1, lists_.Get(printLine).c_str());

      wattroff(window, A_BOLD);
      wattroff(window, A_REVERSE);
   }
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
      client_.LoadPlaylist(lists_.Get(CurrentLine()));
   }
}

uint32_t ListWindow::Current() const
{
   return client_.GetCurrentSong();
}

int32_t ListWindow::DetermineColour(uint32_t line) const
{
   return -1;
}


void ListWindow::AdjustScroll(std::string list)
{
   currentSelection_ = LimitCurrentSelection(currentSelection_);
}


void ListWindow::Clear()
{
   ScrollTo(0);
   lists_.Clear();
}
