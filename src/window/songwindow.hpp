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

   songwindow.hpp - handling of the mpd library but with a more playlist styled interface
   */

#ifndef __UI__SONGWINDOW
#define __UI__SONGWINDOW

// Includes
#include <iostream>

#include "song.hpp"
#include "buffer/browse.hpp"
#include "buffer/library.hpp"
#include "window/selectwindow.hpp"

// Forward Declarations
namespace Main { class Settings; }
namespace Mpc  { class Client; class ClientState; }
namespace Ui   { class Search; }

// Song window class
namespace Ui
{
   class SongWindow : public Ui::SelectWindow
   {
   public:
      SongWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search, std::string name);
      ~SongWindow();

   private:
      SongWindow(SongWindow & song);
      SongWindow & operator=(SongWindow & song);

   public:
      void Print(uint32_t line) const;
      void Left(Ui::Player & player, uint32_t count);
      void Right(Ui::Player & player, uint32_t count);
      void Confirm();

      uint32_t Current() const;
      uint32_t Playlist(int count) const;

      void Add(Mpc::Song * song);
      void AddToPlaylist(uint32_t position);

   public:
      std::string SearchPattern(uint32_t id) const;

   public:
      void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void AddAllLines();
      void CropLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void CropAllLines();
      void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void DeleteAllLines();
      void Edit();
#ifdef LYRICS_SUPPORT
      void Lyrics();
#endif
      void ScrollToFirstMatch(std::string const & input);

   public:
      void Save(std::string const & name);

   public:
      virtual Main::Buffer<Mpc::Song *> & Buffer() { return browse_; }
      virtual Main::Buffer<Mpc::Song *> const & Buffer() const { return browse_; }

   protected:
      virtual void PrintBlankId() const;
      virtual void PrintId(uint32_t Id) const;
      Main::WindowBuffer const & WindowBuffer() const { return browse_; }
      void Clear();

      int32_t DetermineColour(uint32_t line) const;

   private:
      uint32_t GetPositions(int64_t & pos1, int64_t & pos2) const;

   private:
      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Mpc::ClientState     & clientState_;
      Ui::Search     const & search_;
      Mpc::Browse            browse_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
