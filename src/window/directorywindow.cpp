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
#include "clientstate.hpp"
#include "error.hpp"
#include "mpdclient.hpp"
#include "regex.hpp"
#include "screen.hpp"
#include "settings.hpp"
#include "songwindow.hpp"

#include "buffer/playlist.hpp"
#include "mode/search.hpp"
#include "window/debug.hpp"

#include <algorithm>

using namespace Ui;

DirectoryWindow::DirectoryWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Directory & directory, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search) :
   SelectWindow       (settings, screen, "directory"),
   settings_          (settings),
   client_            (client),
   clientState_       (clientState),
   search_            (search),
   directory_         (directory),
   selectedDirectory_ (0)
{
   SoftRedrawOnSetting(Setting::ShowPath);
   SoftRedrawOnSetting(Setting::ShowLists);
}

DirectoryWindow::~DirectoryWindow()
{
   Clear();
}


void DirectoryWindow::Redraw()
{
   directory_.ChangeDirectory(directory_.CurrentDirectory());
   SoftRedraw();
}

void DirectoryWindow::SoftRedraw()
{
   directory_.ChangeDirectory(directory_.CurrentDirectory());
   ScrollTo(CurrentLine());
}

uint32_t DirectoryWindow::Current() const
{
   int32_t current         = CurrentLine();
   int32_t currentSongId   = clientState_.GetCurrentSongPos();
   Mpc::Song * currentSong = NULL;

   if ((currentSongId >= 0) && (currentSongId < static_cast<int32_t>(Main::Playlist().Size())))
   {
      currentSong = Main::Playlist().Get(currentSongId);
   }

   if ((currentSong != NULL))
   {
      for (int32_t i = 0; i < static_cast<int32_t>(directory_.Size()); ++i)
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
   int32_t currentSongId   = clientState_.GetCurrentSongPos();
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
   }
}


void DirectoryWindow::Scroll(int32_t scrollCount)
{
   SelectWindow::Scroll(scrollCount);

   if (settings_.Get(Setting::ShowPath) == true)
   {
      if (currentLine_ >= scrollLine_ - 1)
      {
         ScrollWindow::Scroll(1);
      }
   }
}

void DirectoryWindow::ScrollTo(uint32_t scrollLine)
{
   int64_t oldSelection = currentLine_;
   currentLine_    = (static_cast<int64_t>(scrollLine));

   if (settings_.Get(Setting::ShowPath) == true)
   {
      if ((currentLine_ >= scrollLine_ - 1) || (currentLine_ < (scrollLine_ - Rows())))
      {
         ScrollWindow::ScrollTo(scrollLine);
      }
   }

   LimitCurrentSelection();

   SelectWindow::ScrollTo(scrollLine);
}

void DirectoryWindow::LimitCurrentSelection()
{
   if (settings_.Get(Setting::ShowPath) == true)
   {
      if ((currentLine_ >= static_cast<int32_t>(BufferSize() - 1)) && (BufferSize() > 0))
      {
         currentLine_ = BufferSize() - 2;
      }
   }

   SelectWindow::LimitCurrentSelection();
}


std::string DirectoryWindow::SearchPattern(uint32_t id) const
{
   //! \todo add a search that searches in collapsed songs and
   //! expands things as necessary
   std::string pattern("");

   if (id < directory_.Size())
   {
      Mpc::DirectoryEntry const * const entry = directory_.Get(id);

      switch (entry->type_)
      {
         case Mpc::PathType:
         case Mpc::SongType:
         case Mpc::PlaylistType:
            pattern = entry->name_;
            break;

         case Mpc::ArtistType:
         case Mpc::AlbumType:
         default:
            ASSERT(false);
            break;
      }
   }

   return pattern;
}


void DirectoryWindow::Clear()
{
   directory_.Clear(true);
}

void DirectoryWindow::Print(uint32_t line) const
{
   std::string const BlankLine(Columns(), ' ');

   uint32_t printLine = (line + FirstLine());
   WINDOW * window    = N_WINDOW();

   if ((line == 0) && (settings_.Get(Setting::ShowPath) == true))
   {
      int32_t colour = settings_.colours.Directory;

      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattron(window, COLOR_PAIR(colour));
      }

      wattron(window, A_BOLD);
      std::string const Directory = "/" + directory_.CurrentDirectory();
      mvwprintw(window, line, 0, BlankLine.c_str());
      mvwprintw(window, line, 1, Directory.c_str());
      wattroff(window, A_BOLD);

      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattroff(window, COLOR_PAIR(colour));
      }
   }
   else
   {
      if (settings_.Get(Setting::ShowPath) == true)
      {
         printLine--;
      }

      if (printLine < directory_.Size())
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
      else
      {
         mvwprintw(window, line, 0, BlankLine.c_str());
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

         if (directory_.Get(0)->name_ != "..")
         {
            ScrollTo(selectedDirectory_);
         }
         else
         {
            ScrollTo(0);
         }
      }
   }
}

void DirectoryWindow::Right(Ui::Player & player, uint32_t count)
{
   if (CurrentLine() < directory_.Size())
   {
      if (directory_.Get(0)->name_ != "..")
      {
         selectedDirectory_ = CurrentLine();
      }

      directory_.ChangeDirectory(*directory_.Get(CurrentLine()));
      ScrollTo(0);
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

   if (clientState_.Connected() == true)
   {
      std::vector<uint32_t> Positions = PositionVector(line, count, (pos1 != pos2));

      if (scroll == true)
      {
         ScrollTo(line);
      }

      if (settings_.Get(Setting::AddPosition) == Setting::AddEnd)
      {
         ForPositions(Positions.begin(), Positions.end(), &Mpc::Directory::AddToPlaylist);
      }
      else
      {
         ForPositions(Positions.rbegin(), Positions.rend(), &Mpc::Directory::AddToPlaylist);
      }
   }

   SelectWindow::AddLine(line, count, scroll);
}

void DirectoryWindow::AddAllLines()
{
   if (clientState_.Connected() == true)
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
   DeleteLine(CurrentLine(), directory_.Size() - CurrentLine(), false);
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

   if (clientState_.Connected() == true)
   {
      std::vector<uint32_t> Positions = PositionVector(line, count, (pos1 != pos2));

      if (scroll == true)
      {
         ScrollTo(line);
      }

      ForPositions(Positions.begin(), Positions.end(), &Mpc::Directory::RemoveFromPlaylist);
   }

   SelectWindow::DeleteLine(line, count, scroll);
}

void DirectoryWindow::DeleteAllLines()
{
   if (Main::Playlist().Size() > 0)
   {
      Main::PlaylistPasteBuffer().Clear();
   }

   client_.Clear();
   Main::Playlist().Clear();
}

void DirectoryWindow::Edit()
{
   if (CurrentLine() < directory_.Size())
   {
      Mpc::DirectoryEntry * entry = directory_.Get(CurrentLine());

      if (entry->type_ == Mpc::SongType)
      {
         screen_.CreateSongInfoWindow(entry->song_);
      }
      else if (entry->type_ == Mpc::PlaylistType)
      {
         std::string const path((entry->path_ == "") ? "" : entry->path_ + "/");
         std::string const playlist(path + entry->name_);
         client_.PlaylistContents(playlist);
      }
   }
}

#ifdef LYRICS_SUPPORT
void DirectoryWindow::Lyrics()
{
   if (CurrentLine() < directory_.Size())
   {
      Mpc::DirectoryEntry * entry = directory_.Get(CurrentLine());

      if (entry->type_ == Mpc::SongType)
      {
         screen_.CreateSongLyricsWindow(entry->song_);
      }
   }
}
#endif


void DirectoryWindow::ScrollToFirstMatch(std::string const & input)
{
   for (uint32_t i = 0; i < directory_.Size(); ++i)
   {
      Mpc::DirectoryEntry * entry = directory_.Get(i);

      if ((entry->type_ == Mpc::PathType) &&
          (Algorithm::imatch(entry->name_, input, settings_.Get(Setting::IgnoreTheSort), settings_.Get(Setting::IgnoreCaseSort)) == true))
      {
         ScrollTo(i);
         break;
      }
   }
}


std::vector<uint32_t> DirectoryWindow::PositionVector(uint32_t & line, uint32_t count, bool visual)
{
   Mpc::DirectoryEntry * previous = NULL;

   uint32_t total = 0;
   uint32_t i     = line;

   std::vector<uint32_t> Positions;

   for (i = line; ((total <= count) && (i < directory_.Size())); ++i)
   {
      Mpc::DirectoryEntry * current = directory_.Get(i);

      ++total;

      if (total <= count)
      {
         Positions.push_back(i);
         previous = current;
      }
   }

   line = (i - 1);
   return Positions;
}



template <typename T>
void DirectoryWindow::ForPositions(T start, T end, DirectoryFunction function)
{
   for (T it = start; it != end; ++it)
   {
      (directory_.*function)(Mpc::Song::Single, client_, clientState_, *it);
   }
}


uint32_t DirectoryWindow::BufferSize() const
{
   uint32_t size = directory_.Size();

   if (settings_.Get(Setting::ShowPath) == true)
   {
      ++size;
   }

   return size;
}


int32_t DirectoryWindow::DetermineSongColour(Mpc::DirectoryEntry const * const entry) const
{
   int32_t colour = settings_.colours.Song;

   if ((entry->song_ != NULL) && (entry->song_->URI() == clientState_.GetCurrentSongURI()))
   {
      colour = settings_.colours.CurrentSong;
   }
   else if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
            (search_.HighlightSearch() == true))
   {
      Regex::RE expression(".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

      if (expression.CompleteMatch(entry->name_) == true)
      {
         colour = settings_.colours.SongMatch;
      }
   }

   if (colour == settings_.colours.Song)
   {
      if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL) && (entry->song_->Reference() > 0))
      {
         colour = settings_.colours.FullAdd;
      }
      else if (entry->type_ == Mpc::PathType)
      {
         uint32_t const TotalReferences = directory_.TotalReferences(entry->path_);

         if ((entry->type_ == Mpc::PathType) && (TotalReferences > 0))
         {
            colour = settings_.colours.PartialAdd;

            if (TotalReferences == directory_.AllChildSongs(entry->path_).size())
            {
               colour = settings_.colours.FullAdd;
            }
         }
      }
   }

   return colour;
}
/* vim: set sw=3 ts=3: */
