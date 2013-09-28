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
#include "window/songwindow.hpp"

// Forward Declarations
namespace Main { class Settings; }
namespace Mpc  { class Client; class ClientState; }
namespace Ui   { class Search; }

// Browse window class
namespace Ui
{
   class BrowseWindow : public Ui::SongWindow
   {
   public:
      BrowseWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Browse & browse, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search);
      ~BrowseWindow();

   private:
      BrowseWindow(BrowseWindow & browse);
      BrowseWindow & operator=(BrowseWindow & browse);

   public:
      void Redraw();

   protected:
      void PrintId(uint32_t Id) const;
      Main::WindowBuffer const & WindowBuffer() const { return browse_; }

   private:
      void      SoftRedraw();
      void      Clear();
      uint32_t  BufferSize() const   { return browse_.Size(); }
      Main::Buffer<Mpc::Song *> & Buffer() { return browse_; }
      Main::Buffer<Mpc::Song *> const & Buffer() const { return browse_; }

   private:
      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Mpc::ClientState &     clientState_;
      Ui::Search     const & search_;
      Mpc::Browse &          browse_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
