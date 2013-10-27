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

   playlistwindow.hpp - handling of the mpd playlist interface
   */

#ifndef __UI__PLAYLISTWINDOW
#define __UI__PLAYLISTWINDOW

// Includes
#include <iostream>

#include "song.hpp"
#include "buffer/playlist.hpp"
#include "window/songwindow.hpp"

// Forward Declarations
namespace Main { class Settings; }
namespace Mpc  { class Client; class ClientState; }
namespace Ui   { class Search; }

// Playlist window class
namespace Ui
{
   class PlaylistWindow : public Ui::SongWindow
   {
   public:
      PlaylistWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Playlist & playlist, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search);
      ~PlaylistWindow();

   private:
      PlaylistWindow(PlaylistWindow & playlist);
      PlaylistWindow & operator=(PlaylistWindow & playlist);

   public:
      void Left(Ui::Player & player, uint32_t count);
      void Right(Ui::Player & player, uint32_t count);
      void Redraw();
      void ScrollToFirstMatch(std::string const & input) { }

      uint32_t Current() const;

   public:
      void AdjustScroll(Mpc::Song * song);

   public:
      void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void AddAllLines();
      void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void DeleteAllLines();

   public:
      void Save(std::string const & name);

   protected:
      void PrintId(uint32_t Id) const;
      Main::WindowBuffer const & WindowBuffer() const { return playlist_; }

   private:
      void     Clear();
      uint32_t BufferSize() const { return playlist_.Size(); }
      int32_t  DetermineColour(uint32_t line) const;

   public:
      Main::Buffer<Mpc::Song *> & Buffer() { return playlist_; }
      Main::Buffer<Mpc::Song *> const & Buffer() const { return playlist_; }

   private:
      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Mpc::ClientState &     clientState_;
      Ui::Search     const & search_;
      Mpc::Playlist        & playlist_;
      Mpc::Playlist        & pasteBuffer_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
