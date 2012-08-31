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

   librarywindow.cpp - handling of the mpd music library
   */

#include "librarywindow.hpp"

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

#include <algorithm>
#include <pcrecpp.h>

using namespace Ui;

LibraryWindow::LibraryWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (settings, screen, "library"),
   settings_        (settings),
   client_          (client),
   search_          (search),
   library_         (Main::Library()),

   ignoreCase_      (false),
   ignoreThe_       (false),
   expandArtist_    (false)
{
   ignoreCase_   = settings_.Get(Setting::IgnoreCaseSort);
   ignoreThe_    = settings_.Get(Setting::IgnoreTheSort);
   expandArtist_ = settings_.Get(Setting::ExpandArtists);
}

LibraryWindow::~LibraryWindow()
{
   Clear();
}


void LibraryWindow::Redraw()
{
   Main::Playlist().Clear();
   Clear();
   client_.GetAllMetaInformation();
   client_.ForEachLibrarySong(library_, &Mpc::Library::Add);
   SoftRedraw();

   client_.ForEachQueuedSong(Main::Playlist(), static_cast<void (Mpc::Playlist::*)(Mpc::Song *)>(&Mpc::Playlist::Add));
}

void LibraryWindow::SoftRedraw()
{
   // The library needs to be completely collapsed before sorting as the sort cannot compare different types
   // so we mark everything as collapsed then remove anything that is not an artist from the buffer
   Main::CallbackFunction<Mpc::LibraryEntry *> Callback(&Mpc::MarkUnexpanded);
   library_.ForEachParent(&Callback);

   for (unsigned int i = 0; i < library_.Size(); )
   {
      if (library_.Get(i)->type_ != Mpc::ArtistType)
      {
         library_.Remove(i, 1);
      }
      else
      {
          ++i;
      }
   }

   library_.Sort();

   for (unsigned int i = 0; i < library_.Size(); ++i)
   {
      if (library_.Get(i)->type_ == Mpc::ArtistType)
      {
         if ((settings_.Get(Setting::ExpandArtists) == true) && (library_.Get(i)->expanded_ == false))
         {
            library_.Expand(i);
         }
      }
   }

   screen_.Redraw(Ui::Screen::Browse);
   screen_.Redraw(Ui::Screen::Directory);

   ignoreCase_   = settings_.Get(Setting::IgnoreCaseSort);
   ignoreThe_    = settings_.Get(Setting::IgnoreTheSort);
   expandArtist_ = settings_.Get(Setting::ExpandArtists);

   ScrollTo(CurrentLine());
}

bool LibraryWindow::RequiresRedraw()
{
   return ((ignoreCase_   != settings_.Get(Setting::IgnoreCaseSort)) ||
           (ignoreThe_    != settings_.Get(Setting::IgnoreTheSort))  ||
           (expandArtist_ != settings_.Get(Setting::ExpandArtists)));
}


uint32_t LibraryWindow::Current() const
{
   int32_t current         = CurrentLine();
   int32_t currentSongId   = client_.GetCurrentSong();
   Mpc::Song * currentSong = NULL;

   if ((currentSongId >= 0) && (currentSongId < static_cast<int32_t>(Main::Playlist().Size())))
   {
      currentSong = Main::Playlist().Get(currentSongId);
   }

   if ((currentSong != NULL) && (currentSong->Entry() != NULL))
   {
      Mpc::LibraryEntry * entry = currentSong->Entry();
      current = library_.Index(entry);

      if ((current == -1) && (entry->parent_ != NULL))
      {
         current = library_.Index(entry->parent_);
      }

      if ((current == -1) && (entry->parent_ != NULL) && (entry->parent_->parent_ != NULL))
      {
         current = library_.Index(entry->parent_->parent_);
      }
   }

   return current;
}

std::string LibraryWindow::SearchPattern(int32_t id) const
{
   //! \todo add a search that searches in collapsed songs and
   //! expands things as necessary
   std::string pattern("");

   Mpc::LibraryEntry const * const entry = library_.Get(id);

   switch (entry->type_)
   {
      case Mpc::ArtistType:
         pattern = entry->artist_;
         break;

      case Mpc::AlbumType:
         pattern = entry->album_;
         break;

      case Mpc::SongType:
         pattern = entry->song_->FormatString(settings_.Get(Setting::LibraryFormat));
         break;

      case Mpc::PathType:
      default:
         ASSERT(false);
         break;
   }

   return pattern;
}


void LibraryWindow::Clear()
{
   library_.Clear();
}

void LibraryWindow::Print(uint32_t line) const
{
   std::string const BlankLine(screen_.MaxColumns(), ' ');

   uint32_t printLine = (line + FirstLine());
   WINDOW * window    = N_WINDOW();

   if ((line + FirstLine()) < BufferSize())
   {
      int32_t colour = DetermineSongColour(library_.Get(printLine));

      if (IsSelected(printLine) == true)
      {
         if (settings_.Get(Setting::ColourEnabled) == true)
         {
            wattron(window, COLOR_PAIR(colour));
         }

         wattron(window, A_REVERSE);
      }

      if (library_.Get(printLine)->type_ == Mpc::ArtistType)
      {
         wattron(window, A_BOLD);
      }

      mvwprintw(window, line, 0, BlankLine.c_str());

      uint8_t expandCol = 1;

      if ((library_.Get(printLine)->type_ == Mpc::AlbumType) || (library_.Get(printLine)->type_ == Mpc::ArtistType))
      {
         if (library_.Get(printLine)->type_ == Mpc::AlbumType)
         {
            expandCol += 3;
         }

         if (settings_.Get(Setting::ColourEnabled) == true)
         {
            wattron(window, COLOR_PAIR(colour));
         }

         wmove(window, line, expandCol);

         if (library_.Get(printLine)->type_ == Mpc::ArtistType)
         {
            waddstr(window, library_.Get(printLine)->artist_.c_str());
         }
         else if (library_.Get(printLine)->type_ == Mpc::AlbumType)
         {
            waddstr(window, library_.Get(printLine)->album_.c_str());
         }

         if (settings_.Get(Setting::ColourEnabled) == true)
         {
            wattroff(window, COLOR_PAIR(colour));
         }
      }
      else if ((library_.Get(printLine)->type_ == Mpc::SongType) && (library_.Get(printLine)->song_ != NULL))
      {
         expandCol += 6;
         wmove(window, line, expandCol);

         if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(printLine) == false))
         {
            wattron(window, COLOR_PAIR(REDONDEFAULT));
         }

         wprintw(window, "%5s | " , library_.Get(printLine)->song_->Track().c_str());

         if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(printLine) == false))
         {
            wattroff(window, COLOR_PAIR(REDONDEFAULT));
         }

         PrintSong(line, printLine, colour, settings_.Get(Setting::LibraryFormat), library_.Get(printLine)->song_);
      }

      wattroff(window, A_BOLD | A_REVERSE);

      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattroff(window, COLOR_PAIR(colour));
      }
   }
}

void LibraryWindow::Left(Ui::Player & player, uint32_t count)
{
   if (CurrentLine() < library_.Size())
   {
      Mpc::LibraryEntry * const parent = library_.Get(CurrentLine())->parent_;

      bool scrolled = false;

      if ((library_.Get(CurrentLine())->expanded_ == true) && (library_.Get(CurrentLine())->type_ != Mpc::SongType))
      {
         scrolled = true;
      }

      library_.Collapse(CurrentLine());

      if ((parent == NULL) && (scrolled == false))
      {
         Scroll(-1);
      }
      else if (scrolled == false)
      {
         uint32_t parentLine = 0;
         for (; (parentLine < library_.Size()) && (library_.Get(parentLine) != parent); ++parentLine);

         ScrollTo(parentLine);
      }
   }
}

void LibraryWindow::Right(Ui::Player & player, uint32_t count)
{
   if (CurrentLine() < library_.Size())
   {
      library_.Expand(CurrentLine());
      Scroll(1);
   }
}

void LibraryWindow::Click()
{
   if (CurrentLine() < library_.Size())
   {
      if (library_.Get(CurrentLine())->type_ != Mpc::SongType)
      {
         if (library_.Get(CurrentLine())->expanded_ == false)
         {
            library_.Expand(CurrentLine());
         }
         else
         {
            library_.Collapse(CurrentLine());
         }
      }
   }
}

void LibraryWindow::Confirm()
{
   if (CurrentLine() < library_.Size())
   {
      client_.Clear();
      Main::Playlist().Clear();

      AddLine(CurrentLine(), 1, false);
      client_.Play(0);
   }

   SelectWindow::Confirm();
}


void LibraryWindow::AddLine(uint32_t line, uint32_t count, bool scroll)
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

   DoForLine(&Mpc::Library::AddToPlaylist, line, count, scroll, (pos1 != pos2));
   SelectWindow::AddLine(line, count, scroll);
}

void LibraryWindow::AddAllLines()
{
   if (client_.Connected() == true)
   {
      client_.AddAllSongs();
   }

   ScrollTo(CurrentLine());
}

void LibraryWindow::CropLine(uint32_t line, uint32_t count, bool scroll)
{
   DeleteLine(line, count, scroll);
   SelectWindow::DeleteLine(line, count, scroll);
}

void LibraryWindow::CropAllLines()
{
   DeleteLine(CurrentLine(), BufferSize() - CurrentLine(), false);
}

void LibraryWindow::DeleteLine(uint32_t line, uint32_t count, bool scroll)
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

   DoForLine(&Mpc::Library::RemoveFromPlaylist, line, count, scroll, (pos1 != pos2));
   SelectWindow::DeleteLine(line, count, scroll);
}

void LibraryWindow::DeleteAllLines()
{
   Main::PlaylistPasteBuffer().Clear();
   client_.Clear();
   Main::Playlist().Clear();
}

void LibraryWindow::Edit()
{
   Mpc::LibraryEntry * entry = library_.Get(CurrentLine());

   if (entry->type_ != Mpc::SongType)
   {
      std::string title = library_.Get(CurrentLine())->album_;
      title = (title != "") ? title : library_.Get(CurrentLine())->artist_;

      SongWindow * window = screen_.CreateSongWindow("L:" + title);

      Main::CallbackObject<Ui::SongWindow, Mpc::Song * > callback(*window, &Ui::SongWindow::Add);

      // Do not need to sort as this ensures it will be sorted in the same order as the library
      Main::Library().ForEachChild(CurrentLine(), &callback);

      if (window->ContentSize() > -1)
      {
         screen_.SetActiveAndVisible(screen_.GetWindowFromName(window->Name()));
      }
      else
      {
         screen_.SetVisible(screen_.GetWindowFromName(window->Name()), false);
      }
   }
   else
   {
      screen_.CreateSongInfoWindow(entry->song_);
   }
}


void LibraryWindow::ScrollToFirstMatch(std::string const & input)
{
   for (uint32_t i = 0; i < BufferSize(); ++i)
   {
      Mpc::LibraryEntry * entry = library_.Get(i);

      if ((entry->type_ == Mpc::ArtistType) &&
          (Algorithm::imatch(entry->artist_, input, settings_.Get(Setting::IgnoreTheSort), settings_.Get(Setting::IgnoreCaseSort)) == true))
      {
         ScrollTo(i);
         break;
      }
   }
}


void LibraryWindow::DoForLine(LibraryFunction function, uint32_t line, uint32_t count, bool scroll, bool countskips)
{
   if (client_.Connected() == true)
   {
      Mpc::LibraryEntry * previous = NULL;

      uint32_t total = 0;
      uint32_t i     = line;

      {
         Mpc::CommandList list(client_, (count > 1));

         for (i = line; ((total <= count) && (i < BufferSize())); ++i)
         {
            Mpc::LibraryEntry * current = library_.Get(i);

            if ((previous == NULL) ||
               ((current->Parent() != previous) &&
               ((current->Parent() == NULL) || (current->Parent()->Parent() != previous))))
            {
               ++total;

               if (total <= count)
               {
                  (library_.*function)(Mpc::Song::Single, client_, i);
                  previous = current;
               }
            }
            else if (countskips == true)
            {
               ++total;
            }
         }
      }

      if (scroll == true)
      {
         Scroll(i - line - 1);
      }
   }
}


int32_t LibraryWindow::DetermineSongColour(Mpc::LibraryEntry const * const entry) const
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

      if (((entry->type_ == Mpc::ArtistType) && (expression.FullMatch(entry->artist_) == true)) ||
          ((entry->type_ == Mpc::AlbumType)  && (expression.FullMatch(entry->album_) == true)) ||
          ((entry->type_ == Mpc::SongType)   && (expression.FullMatch(entry->song_->FormatString(settings_.Get(Setting::LibraryFormat))) == true)))
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
      else if (entry->type_ != Mpc::SongType)
      {
         if ((entry->children_.size() >= 1) && (entry->childrenInPlaylist_ == static_cast<int32_t>(entry->children_.size())))
         {
            colour = Colour::FullAdd;
         }
         else if ((entry->children_.size() >= 1) && (entry->partial_ > 0))
         {
            colour = Colour::PartialAdd;
         }
      }
   }

   return colour;
}
/* vim: set sw=3 ts=3: */
