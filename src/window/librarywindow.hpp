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

   librarywindow.hpp - handling of the mpd music library
   */

#ifndef __UI__LIBRARYWINDOW
#define __UI__LIBRARYWINDOW

#include "song.hpp"
#include "buffer/library.hpp"
#include "window/selectwindow.hpp"

#include <map>

namespace Main { class Settings; }
namespace Mpc  { class Client; class ClientState; }

namespace Ui
{
   class Search;

   class LibraryWindow : public Ui::SelectWindow
   {
   private:
      typedef void (Mpc::Library::*LibraryFunction)(Mpc::Song::SongCollection Collection, Mpc::Client & client, Mpc::ClientState & clientState, uint32_t position);

   public:
      LibraryWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Library & library, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search);
      ~LibraryWindow();

   private:
      LibraryWindow(LibraryWindow & library);
      LibraryWindow & operator=(LibraryWindow & library);

   private:
      void Print(uint32_t line) const;

   public:
      void Left(Ui::Player & player, uint32_t count);
      void Right(Ui::Player & player, uint32_t count);
      void Click();
      void Confirm();
      void Redraw();

      uint32_t Current() const;

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

   protected:
      Main::WindowBuffer const & WindowBuffer() const { return library_; }

   private:
      std::vector<uint32_t> PositionVector(uint32_t & line, uint32_t count, bool visual);

      template <typename T>
      void ForPositions(T start, T end, LibraryFunction function);

   private:
      void      SoftRedraw();
      void      Clear();
      uint32_t  BufferSize() const { return library_.Size(); }
      int32_t   DetermineColour(uint32_t line) const;

   private:
      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Mpc::ClientState     & clientState_;
      Ui::Search     const & search_;
      Mpc::Library         & library_;
   };
}
#endif
/* vim: set sw=3 ts=3: */
