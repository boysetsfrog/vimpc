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

#include "buffers.hpp"
#include "mpdclient.hpp"
#include "events.hpp"
#include "screen.hpp"
#include "settings.hpp"
#include "songsorter.hpp"
#include "vimpc.hpp"

#include "buffer/library.hpp"
#include "mode/search.hpp"
#include "window/console.hpp"

using namespace Ui;

BrowseWindow::BrowseWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Browse & browse, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search) :
   SongWindow       (settings, screen, client, clientState, search, "browse"),
   settings_        (settings),
   client_          (client),
   clientState_     (clientState),
   search_          (search),
   browse_          (browse)
{
   SoftRedrawOnSetting(Setting::IgnoreCaseSort);
   SoftRedrawOnSetting(Setting::IgnoreTheSort);
   SoftRedrawOnSetting(Setting::Sort);
   SoftRedrawOnSetting(Setting::AlbumArtist);
}

BrowseWindow::~BrowseWindow()
{
}

void BrowseWindow::Redraw()
{
   Clear();
   SoftRedraw();
}

void BrowseWindow::SoftRedraw()
{
   std::string uri = "";

   if (browse_.Size() > 0)
   {
      Mpc::Song * song = browse_.Get(CurrentLine());
      uri = (song != NULL) ? song->URI() : "";
   }

   std::string const format = settings_.Get(::Setting::Sort);

   if ((format == "library") || (browse_.Size() == 0))
   {
      Clear();
      Main::Library().ForEachSong([this] (Mpc::Song * song) { Add(song); });
   }

   if (format != "library")
   {
      Ui::SongSorter const sorter(format);
      Buffer().Sort(sorter);
   }

   // If we are redrawing and can keep the same scroll point do so
   // otherwise if we are redrawing due to a new playlist load etc, we need to scroll to the start
   if (uri != "")
   {
      for (int i = 0; i < browse_.Size(); ++i)
      {
         if ((browse_.Get(i) != NULL) && (browse_.Get(i)->URI() == uri))
         {
            ScrollTo(i);
         }
      }
   }
   else
   {
      ScrollTo(0);
   }
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
