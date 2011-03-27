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

   librarywindow.cpp - handling of the mpd music library 
   */

#include "librarywindow.hpp"

#include "colour.hpp"
#include "mpdclient.hpp"
#include "screen.hpp"
#include "search.hpp"
#include "settings.hpp"

#include <algorithm>
#include <boost/regex.hpp>

using namespace Ui;

LibraryWindow::LibraryWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (screen),
   settings_        (settings),
   client_          (client),
   search_          (search),
   library_         (Mpc::Library::Instance())
{
}

LibraryWindow::~LibraryWindow()
{
   Clear();
}


void LibraryWindow::Redraw()
{
   Clear();
   client_.ForEachLibrarySong(library_, &Mpc::Library::Add);

   library_.Sort();
}



std::string LibraryWindow::SearchPattern(UNUSED int32_t id)
{
   //! \todo add a search that searches in collapsed songs and
   //! expands things as necessary
   std::string pattern("");

   Mpc::LibraryEntry const * const entry = library_.Entry(id);

   switch (entry->type_)
   {
      case Mpc::ArtistType:
         pattern = entry->artist_;
         break;
      
      case Mpc::AlbumType:
         pattern = entry->album_;
         break;

      case Mpc::SongType:
         pattern = entry->song_->Title();
         break;

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
   static std::string const BlankLine(screen_.MaxColumns(), ' ');

   WINDOW * window = N_WINDOW();

   if ((line + FirstLine()) < BufferSize())
   {
      uint32_t printLine = (line + FirstLine());

      int colour = DetermineSongColour(library_.Entry(printLine));

      if (printLine == CurrentLine())
      {
         wattron(window, COLOR_PAIR(colour));
         wattron(window, A_REVERSE);
      }

      mvwprintw(window, line, 0, BlankLine.c_str());

      if ((library_.Entry(printLine)->type_ == Mpc::AlbumType) || (library_.Entry(printLine)->type_ == Mpc::ArtistType))
      {
         uint8_t expandCol = 0;

         if (library_.Entry(printLine)->type_ == Mpc::AlbumType)
         {
            mvwprintw(window, line, 1, "|--");
            expandCol = 4;
         }
         
         wattron(window, A_BOLD); 
         mvwprintw(window, line, expandCol, "[ ]");

         if (printLine != CurrentLine()) 
         { 
            wattron(window, COLOR_PAIR(REDONDEFAULT)); 
         }
         
         char expand = (library_.Entry(printLine)->expanded_ == true) ? '-' : '+';
         mvwprintw(window, line, expandCol + 1, "%c", expand);

         if (printLine != CurrentLine()) 
         { 
            wattroff(window, COLOR_PAIR(REDONDEFAULT)); 
         }

         wattroff(window, A_BOLD);
         wattron(window, COLOR_PAIR(colour));
         wmove(window, line, expandCol + 4);

         if (library_.Entry(printLine)->type_ == Mpc::ArtistType)
         {
            waddstr(window, library_.Entry(printLine)->artist_.c_str());
         }
         else if (library_.Entry(printLine)->type_ == Mpc::AlbumType)
         {
            waddstr(window, library_.Entry(printLine)->album_.c_str());
         }

         wattroff(window, COLOR_PAIR(colour));
      }
      else if ((library_.Entry(printLine)->type_ == Mpc::SongType) && (library_.Entry(printLine)->song_ != NULL))
      {
         mvwprintw(window, line, 1, "|   |--");

         wattron(window, A_BOLD);
         wprintw(window, "[");

         if (printLine != CurrentLine()) 
         {
            wattron(window, COLOR_PAIR(REDONDEFAULT)); 
         }

         wprintw(window, "%2s" , library_.Entry(printLine)->song_->Track().c_str());

         if (printLine != CurrentLine()) 
         {
            wattroff(window, COLOR_PAIR(REDONDEFAULT)); 
         }
         
         wprintw(window, "]");

         if (colour != CURRENTSONGCOLOUR)
         {
            wattroff(window, A_BOLD);
         }

         waddstr(window, " ");
         wattron(window, COLOR_PAIR(colour));

         std::string title = library_.Entry(printLine)->song_->Title();

         if (title == "Unknown")
         {
            title = library_.Entry(printLine)->song_->URI();
         }

         if (title.length() >= 48)
         {
            title  = title.substr(0, 45);
            title += "...";
         }

         waddstr(window, title.c_str());

         mvwprintw(window, line, 61, " [");
         waddstr(window, library_.Entry(printLine)->song_->DurationString().c_str());
         waddstr(window, "]");
      }

      wattroff(window, A_BOLD | A_REVERSE | COLOR_PAIR(colour));
   }
}

void LibraryWindow::Left(UNUSED Ui::Player & player, UNUSED uint32_t count)
{
   Mpc::LibraryEntry * const parent = library_.Entry(CurrentLine())->parent_;

   library_.Collapse(CurrentLine());

   if (parent == NULL)
   {
      Scroll(-1);
   }
   else
   {
      uint32_t parentLine = 0;
      for (; (parentLine < library_.Entries()) && (library_.Entry(parentLine) != parent); ++parentLine);

      ScrollTo(parentLine);
   }
}

void LibraryWindow::Right(UNUSED Ui::Player & player, UNUSED uint32_t count)
{
   library_.Expand(CurrentLine());
   Scroll(1);
}

void LibraryWindow::Confirm()
{
   client_.Clear();
   
   library_.AddToPlaylist(client_, Mpc::Song::Single, CurrentLine());
   client_.Play(0);
   screen_.Redraw(Ui::Screen::Playlist);
}

// \todo should store the current state for each album artist entry in the library
// and update it when there is an add or delete that way i won't need to do a recursive loop
// here for every single print, which is plenty slow
int32_t LibraryWindow::DetermineSongColour(Mpc::LibraryEntry const * const entry) const
{
   int32_t colour = SONGCOLOUR;

   if ((search_.LastSearchString() != "") && (settings_.HightlightSearch() == true))
   {
      boost::regex expression(".*" + search_.LastSearchString() + ".*");

      if (((entry->type_ == Mpc::ArtistType) && (boost::regex_match(entry->artist_, expression))) ||
          ((entry->type_ == Mpc::AlbumType)  && (boost::regex_match(entry->album_,  expression))) ||
          ((entry->type_ == Mpc::SongType)   && (boost::regex_match(entry->song_->Title(), expression))))
      {
         colour = SONGMATCHCOLOUR;
      } 
   }

   //! \todo this needs to be dramatically improved in speed it really is a PoC at the moment
   //        and is way to slow to be usable in anyway
   if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL) && (client_.SongIsInQueue(*entry->song_) == true))
   {
      colour = GREENONDEFAULT;
   }
   else if ((entry->type_ != Mpc::SongType) && (entry->children_.size() > 0))
   {
      Mpc::LibraryEntryVector::const_iterator it = entry->children_.begin();

      unsigned int count = 0;

      for (; (it != entry->children_.end()); ++it)
      {
         int32_t newColour = DetermineSongColour(*it);

         if ((newColour == GREENONDEFAULT) || (newColour == CURRENTSONGCOLOUR) || (newColour == CYANONDEFAULT))
         {
            if ((newColour == GREENONDEFAULT) || (newColour == CURRENTSONGCOLOUR))
            {
               count++;
            }
            colour = CYANONDEFAULT;
         }
      }

      if (count == entry->children_.size())
      {
         colour = GREENONDEFAULT;
      }
   }
   
   if ((entry->song_ != NULL) && (entry->song_->URI() == client_.GetCurrentSongURI()))
   {
      colour = CURRENTSONGCOLOUR;
   }

   return colour;
}
