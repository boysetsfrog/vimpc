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
#include "screen.hpp"
#include "settings.hpp"
#include "songsorter.hpp"
#include "buffer/library.hpp"
#include "mode/search.hpp"
#include "window/console.hpp"

using namespace Ui;

BrowseWindow::BrowseWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Ui::Search const & search) :
   SongWindow       (settings, screen, client, search, "browse"),
   settings_        (settings),
   client_          (client),
   search_          (search),
   browse_          (Main::Browse())
{
   SoftRedrawOnSetting(Setting::IgnoreCaseSort);
   SoftRedrawOnSetting(Setting::IgnoreTheSort);
   SoftRedrawOnSetting(Setting::Sort);
}

BrowseWindow::~BrowseWindow()
{
}

void BrowseWindow::Redraw()
{
   Clear();
   Main::CallbackObject<Ui::BrowseWindow, Mpc::Song * > callback(*this, &Ui::BrowseWindow::Add);
   Main::Library().ForEachSong(&callback);
   SoftRedraw();
}

void BrowseWindow::SoftRedraw()
{
   Ui::SongSorter const sorter(settings_.Get(::Setting::Sort));
   Buffer().Sort(sorter);
   ScrollTo(0);
}

void BrowseWindow::PrintId(uint32_t Id) const
{
   if (settings_.Get(Setting::BrowseNumbers) == true)
   {
      SongWindow::PrintId(Id);
   }
   else
   {
      SongWindow::PrintBlankId();
   }
}


void BrowseWindow::Clear()
{
   ScrollTo(0);
   browse_.Clear();
}
/* vim: set sw=3 ts=3: */
