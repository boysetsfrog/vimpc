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

   playlist.hpp - handling of the mpd playlist interface 
   */

#ifndef __UI__PLAYLIST
#define __UI__PLAYLIST

#include "song.hpp"
#include "selectwindow.hpp"

namespace Main
{
   class Settings;
}

namespace Mpc
{
   class Client;
}

namespace Ui
{
   class Search;

   class PlaylistWindow : public Ui::SelectWindow
   {
   public:
      PlaylistWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search);
      ~PlaylistWindow();

   public:
      void AddSong(Mpc::Song const * const newSong);
      Mpc::Song const * GetSong(uint32_t songIndex);

   public:
      uint32_t GetCurrentSong()     const;
      uint32_t TotalNumberOfSongs() const;

   public:
      void Print(uint32_t line) const;
      void Confirm();
      void Redraw();

   private:
      void DeleteSongs();
      void Clear();

   private:
      size_t BufferSize() const { return buffer_.size(); }

   private:
      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Ui::Search     const & search_;

      typedef std::vector<Mpc::Song *> SongBuffer;
      SongBuffer buffer_;
   };
}

#endif
