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

#include "buffers.hpp"
#include "mpdclient.hpp"
#include "regex.hpp"
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
   lists_           (&lists)
{
   SetSupportsVisual(false);

   SoftRedrawOnSetting(Setting::IgnoreCaseSort);
   SoftRedrawOnSetting(Setting::IgnoreTheSort);
   SoftRedrawOnSetting(Setting::Playlists);

   Main::AllLists().AddCallback(Main::Buffer_Remove,  [this] (Mpc::Lists::BufferType line) { AdjustScroll(line); });
   Main::MpdLists().AddCallback(Main::Buffer_Remove,  [this] (Mpc::Lists::BufferType line) { AdjustScroll(line); });
   Main::FileLists().AddCallback(Main::Buffer_Remove, [this] (Mpc::Lists::BufferType line) { AdjustScroll(line); });

   SoftRedraw();
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
   if (settings_.Get(Setting::Playlists) == Setting::PlaylistsAll)
   {
      lists_ = &Main::AllLists();
   }
   else if (settings_.Get(Setting::Playlists) == Setting::PlaylistsMpd)
   {
      lists_ = &Main::MpdLists();
   }
   else if (settings_.Get(Setting::Playlists) == Setting::PlaylistsFiles)
   {
      lists_ = &Main::FileLists();
   }

   lists_->Sort();
   SetScrollLine(0);
   ScrollTo(0);
}

void ListWindow::Print(uint32_t line) const
{
#if 1
   uint32_t printLine = line + FirstLine();
   WINDOW * window = N_WINDOW();

   if (printLine < BufferSize())
   {
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
      mvwaddstr(window, line, 1, lists_->Get(printLine).name_.c_str());

      wattroff(window, A_REVERSE);

      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattroff(window, COLOR_PAIR(colour));
      }
   }
   else
   {
      std::string const BlankLine(Columns(), ' ');
      mvwprintw(window, line, 0, BlankLine.c_str());
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
   if (lists_->Size() > CurrentLine())
   {
      client_.LoadPlaylist(lists_->Get(CurrentLine()).path_);
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

   if (line < lists_->Size())
   {
      if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
          (search_.HighlightSearch() == true))
      {
         Regex::RE expression (".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

         if (expression.CompleteMatch(lists_->Get(line).name_) == true)
         {
            colour = settings_.colours.SongMatch;
         }
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
   for (uint32_t i = 0; i < count; ++i)
   {
      if (i < lists_->Size())
      {
         client_.AppendPlaylist(lists_->Get(line +  i).path_);
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
      if (i < lists_->Size())
      {
         client_.PlaylistContentsForRemove(lists_->Get(line +  i).path_);
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
         client_.RemovePlaylist(lists_->Get(line).path_);
         lists_->Remove(line, 1);
      }
   }

   lists_->Sort();
   SelectWindow::DeleteLine(line, count, scroll);
}

void ListWindow::CropAllLines()
{
   if (BufferSize() > 0)
   {
      DeleteLine(CurrentLine(), BufferSize() - CurrentLine(), false);
   }
}

void ListWindow::Edit()
{
   if (lists_->Size() > 0)
   {
      Mpc::List const playlist(lists_->Get(CurrentLine()));
      client_.PlaylistContents(playlist.name_);
   }
}

void ListWindow::ScrollToFirstMatch(std::string const & input)
{
   for (uint32_t i = 0; i < lists_->Size(); ++i)
   {
      Mpc::List const entry = lists_->Get(i);

      if ((Algorithm::imatch(entry.name_, input, settings_.Get(Setting::IgnoreTheSort), settings_.Get(Setting::IgnoreCaseSort)) == true))
      {
         ScrollTo(i);
         break;
      }
   }
}


void ListWindow::Clear()
{
   ScrollTo(0);
   lists_->Clear();
}
/* vim: set sw=3 ts=3: */
