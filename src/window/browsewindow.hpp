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

   browsewindow.hpp - handling of the mpd library but with a more playlist styled interface 
   */

#ifndef __UI__BROWSEWINDOW
#define __UI__BROWSEWINDOW

// Includes
#include <iostream>

#include "song.hpp"
#include "buffer/browse.hpp"
#include "buffer/library.hpp"
#include "window/selectwindow.hpp"

// Forward Declarations
namespace Main { class Settings; }
namespace Mpc  { class Client; }
namespace Ui   { class Search; }

// Browse window class
namespace Ui
{
   class BrowseWindow : public Ui::SelectWindow
   {
   public:
      BrowseWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search);
      ~BrowseWindow();

   private:
      BrowseWindow(BrowseWindow & browse);
      BrowseWindow & operator=(BrowseWindow & browse);

   public:
      void Print(uint32_t line) const;
      void Left(Ui::Player & player, uint32_t count);
      void Right(Ui::Player & player, uint32_t count);
      void Confirm();
      void Redraw();

      uint32_t Current() const;
      uint32_t Playlist(int Offset) const;

      void Add(Mpc::Song * song);

   public:
      std::string SearchPattern(int32_t id) { return browse_.Get(id)->PlaylistDescription(); }

   private:
      void    Clear();
      size_t  BufferSize() const { return browse_.Size(); }
      int32_t DetermineSongColour(Mpc::Song const * const nextSong) const;

   private:
      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Ui::Search     const & search_;
      Mpc::Browse          & browse_;
   };
}

#endif
