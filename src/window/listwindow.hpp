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

   listwindow.hpp - handling of multiple mpd playlists
   */

#ifndef __UI__LISTWINDOW
#define __UI__LISTWINDOW

// Includes
#include <iostream>

#include "buffer/list.hpp"
#include "window/selectwindow.hpp"

// Forward Declarations
namespace Main { class Settings; }
namespace Mpc  { class Client; }
namespace Ui   { class Search; }

// List window class
namespace Ui
{
   class ListComparator
   {
      public:
      bool operator() (std::string i, std::string j) { return (i<j); };
   };

   class ListWindow : public Ui::SelectWindow
   {
   public:
      ListWindow(Main::Settings const & settings, Ui::Screen const & screen, Mpc::Client & client, Ui::Search const & search);
      ~ListWindow();

   private:
      ListWindow(ListWindow & window);
      ListWindow & operator=(ListWindow & window);

   public:
      void Print(uint32_t line) const;
      void Left(Ui::Player & player, uint32_t count);
      void Right(Ui::Player & player, uint32_t count);
      void Confirm();
      void Redraw();

      uint32_t Current() const;

   public:
      void AdjustScroll(std::string list);

   public:
      std::string SearchPattern(int32_t id) { return lists_.Get(id); }

   public:
      void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void AddAllLines();
      void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void DeleteAllLines();

   private:
      void    Clear();
      size_t  BufferSize() const { return lists_.Size(); }
      int32_t DetermineColour(uint32_t line) const;

   private:
      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Ui::Search     const & search_;
      Mpc::Lists           & lists_;
   };
}

#endif
