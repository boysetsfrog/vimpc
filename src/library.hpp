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

   library.hpp - handling of the mpd music library 
   */

#ifndef __UI__LIBRARY
#define __UI__LIBRARY

#include "scrollwindow.hpp"
#include "song.hpp"

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

   class LibraryWindow : public Ui::ScrollWindow
   {
   public:
      LibraryWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search);
      ~LibraryWindow();

   public:
      void AddSong(Mpc::Song const * const song);

   public:
      void Print(uint32_t line) const;
      void Confirm() const;
      void Scroll(int32_t scrollCount);
      void ScrollTo(uint16_t scrollLine);
      void Redraw();

      uint16_t CurrentLine() const { return 0; }

   private:
      void Clear();

   private:
      size_t BufferSize() const { return buffer_.size(); }

   private:
      typedef enum
      {
         ArtistType = 0,
         AlbumType,
         SongType
      } EntryType;

      struct LibraryEntry
      {
         EntryType   type_;
         std::string str_;
         Mpc::Song * song_;
         bool        expanded_;
      };

      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Ui::Search     const & search_;

      typedef std::vector<LibraryEntry *> SongBuffer;
      SongBuffer buffer_;
   };
}

#endif
