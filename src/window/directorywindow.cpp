/*
   Vimpc
   Copyright (C) 2010 - 2012 Nathan Sweetman

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

   directorywindow.cpp - navigate the mpd directory
   */

#include "directorywindow.hpp"

#include "buffers.hpp"
#include "buffers.hpp"
#include "callback.hpp"
#include "colour.hpp"
#include "error.hpp"
#include "mpdclient.hpp"
#include "screen.hpp"
#include "settings.hpp"
#include "songwindow.hpp"

#include "buffer/playlist.hpp"
#include "mode/search.hpp"
#include "window/debug.hpp"

#include <algorithm>
#include <pcrecpp.h>

using namespace Ui;

DirectoryWindow::DirectoryWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (settings, screen, "directory"),
   settings_        (settings),
   client_          (client),
   search_          (search),
   directory_       (Main::Directory()),
   redraw_          (false)
{
}

DirectoryWindow::~DirectoryWindow()
{
   Clear();
}


void DirectoryWindow::Redraw()
{
   Clear();
   client_.ForEachDirectory(directory_, &Mpc::Directory::Add);
   client_.ForEachLibrarySong(directory_, &Mpc::Directory::Add);

   SoftRedraw();
}

void DirectoryWindow::SoftRedraw()
{
   ScrollTo(CurrentLine());
}

bool DirectoryWindow::RequiresRedraw()
{
   return redraw_;
}


uint32_t DirectoryWindow::Current() const
{
   int32_t current         = CurrentLine();
   int32_t currentSongId   = client_.GetCurrentSong();
   Mpc::Song * currentSong = NULL;

   if ((currentSongId >= 0) && (currentSongId < static_cast<int32_t>(Main::Playlist().Size())))
   {
      currentSong = Main::Playlist().Get(currentSongId);
   }

   if ((currentSong != NULL))
   {
      for (int i = 0; i < directory_.Size(); ++i)
      {
         if (directory_.Get(i)->song_ == currentSong)
         {
            current = i;
         }
      }
   }

   return current;
}


void DirectoryWindow::ScrollToCurrent()
{
   int32_t currentSongId   = client_.GetCurrentSong();
   Mpc::Song * currentSong = NULL;

   if ((currentSongId >= 0) && (currentSongId < static_cast<int32_t>(Main::Playlist().Size())))
   {
      currentSong = Main::Playlist().Get(currentSongId);
   }

   if ((currentSong != NULL) && (currentSong->Entry() != NULL) && (currentSong->URI() != ""))
   {
      std::string Directory = currentSong->URI();

      if (Directory.find("/") != std::string::npos)
      {
         Directory = Directory.substr(0, Directory.find_last_of("/"));
      }
      else
      {
         Directory = "";
      }

      directory_.ChangeDirectory(Directory);
      redraw_ = true;
   }
}

std::string DirectoryWindow::SearchPattern(int32_t id) const
{
   //! \todo add a search that searches in collapsed songs and
   //! expands things as necessary
   std::string pattern("");

   Mpc::DirectoryEntry const * const entry = directory_.Get(id);

   switch (entry->type_)
   {
      case Mpc::PathType:
         pattern = entry->name_;
         break;

      case Mpc::SongType:
         pattern = entry->song_->URI();
         break;

      default:
         ASSERT(false);
         break;
   }

   return pattern;
}


void DirectoryWindow::Clear()
{
   directory_.Clear(true);
}

void DirectoryWindow::Print(uint32_t line) const
{
   std::string const BlankLine(screen_.MaxColumns(), ' ');

   uint32_t printLine = (line + FirstLine());
   WINDOW * window    = N_WINDOW();

   if ((line + FirstLine()) < BufferSize())
   {
      int32_t colour = DetermineSongColour(directory_.Get(printLine));

      if (IsSelected(printLine) == true)
      {
         if (settings_.Get(Setting::ColourEnabled) == true)
         {
            wattron(window, COLOR_PAIR(colour));
         }

         wattron(window, A_REVERSE);
      }

      mvwprintw(window, line, 0, BlankLine.c_str());

      uint8_t expandCol = 1;

      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattron(window, COLOR_PAIR(colour));
      }

      wmove(window, line, expandCol);
      waddstr(window, directory_.Get(printLine)->name_.c_str());
      Debug(directory_.Get(printLine)->name_.c_str());

      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattroff(window, COLOR_PAIR(colour));
      }

      wattroff(window, A_REVERSE);

      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattroff(window, COLOR_PAIR(colour));
      }
   }
}

void DirectoryWindow::Left(Ui::Player & player, uint32_t count)
{
   if (CurrentLine() < directory_.Size())
   {
      if (directory_.Get(0)->name_ == "..")
      {
         directory_.ChangeDirectory(*directory_.Get(0));
         ScrollTo(0);
         redraw_ = true;
      }
   }
}

void DirectoryWindow::Right(Ui::Player & player, uint32_t count)
{
   if (CurrentLine() < directory_.Size())
   {
      directory_.ChangeDirectory(*directory_.Get(CurrentLine()));
      ScrollTo(0);
      redraw_ = true;
   }
}

void DirectoryWindow::Click()
{
}

void DirectoryWindow::Confirm()
{
   if (CurrentLine() < directory_.Size())
   {
      if (directory_.Get(CurrentLine())->type_ == Mpc::PathType)
      {
         directory_.ChangeDirectory(*directory_.Get(CurrentLine()));
         ScrollTo(0);
         redraw_ = true;
      }
      else
      {
         client_.Clear();
         Main::Playlist().Clear();

         AddLine(CurrentLine(), 1, false);
         client_.Play(0);
         SelectWindow::Confirm();
      }
   }

   SelectWindow::Confirm();
}


void DirectoryWindow::AddLine(uint32_t line, uint32_t count, bool scroll)
{
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

   DoForLine(&Mpc::Directory::AddToPlaylist, line, count, scroll, (pos1 != pos2));
   SelectWindow::AddLine(line, count, scroll);
}

void DirectoryWindow::AddAllLines()
{
   if (client_.Connected() == true)
   {
      client_.AddAllSongs();
   }

   ScrollTo(CurrentLine());
}

void DirectoryWindow::CropLine(uint32_t line, uint32_t count, bool scroll)
{
   DeleteLine(line, count, scroll);
   SelectWindow::DeleteLine(line, count, scroll);
}

void DirectoryWindow::CropAllLines()
{
   DeleteLine(CurrentLine(), BufferSize() - CurrentLine(), false);
}

void DirectoryWindow::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
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

   DoForLine(&Mpc::Directory::RemoveFromPlaylist, line, count, scroll, (pos1 != pos2));
   SelectWindow::DeleteLine(line, count, scroll);
}

void DirectoryWindow::DeleteAllLines()
{
   Main::PlaylistPasteBuffer().Clear();
   client_.Clear();
   Main::Playlist().Clear();
}

void DirectoryWindow::Edit()
{
   Mpc::DirectoryEntry * entry = directory_.Get(CurrentLine());

   if (entry->type_ == Mpc::SongType)
   {
      screen_.CreateSongInfoWindow(entry->song_);
   }
}


void DirectoryWindow::ScrollToFirstMatch(std::string const & input)
{
   for (uint32_t i = 0; i < BufferSize(); ++i)
   {
      Mpc::DirectoryEntry * entry = directory_.Get(i);

      if ((entry->type_ == Mpc::ArtistType) &&
          (Algorithm::imatch(entry->name_, input, settings_.Get(Setting::IgnoreTheSort), settings_.Get(Setting::IgnoreCaseSort)) == true))
      {
         ScrollTo(i);
         break;
      }
   }
}


void DirectoryWindow::DoForLine(DirectoryFunction function, uint32_t line, uint32_t count, bool scroll, bool countskips)
{
   if (client_.Connected() == true)
   {
      Mpc::DirectoryEntry * previous = NULL;

      uint32_t total = 0;
      uint32_t i     = line;

      {
         Mpc::CommandList list(client_, (count > 1));

         for (i = line; ((total <= count) && (i < BufferSize())); ++i)
         {
            Mpc::DirectoryEntry * current = directory_.Get(i);

            ++total;

            if (total <= count)
            {
               (directory_.*function)(Mpc::Song::Single, client_, i);
               previous = current;
            }
         }
      }

      if (scroll == true)
      {
         Scroll(i - line - 1);
      }
   }
}


int32_t DirectoryWindow::DetermineSongColour(Mpc::DirectoryEntry const * const entry) const
{
   int32_t colour = Colour::Song;

   if ((entry->song_ != NULL) && (entry->song_->URI() == client_.GetCurrentSongURI()))
   {
      colour = Colour::CurrentSong;
   }
   else if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
            (search_.HighlightSearch() == true))
   {
      pcrecpp::RE expression(".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

      if (((entry->type_ == Mpc::PathType) && (expression.FullMatch(entry->name_) == true)) ||
          ((entry->type_ == Mpc::SongType)   && (expression.FullMatch(entry->song_->URI()) == true)))
      {
         colour = Colour::SongMatch;
      }
   }

   if (colour == Colour::Song)
   {
      if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL) && (entry->song_->Reference() > 0))
      {
         colour = Colour::FullAdd;
      }
   }

   return colour;
}
/* vim: set sw=3 ts=3: */
