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
   class ListWindow : public Ui::SelectWindow
   {
   public:
      ListWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Lists & lists, Mpc::Client & client, Ui::Search const & search);
      ~ListWindow();

   private:
      ListWindow(ListWindow & window);
      ListWindow & operator=(ListWindow & window);

   private:
      void Print(uint32_t line) const;

   public:
      void Left(Ui::Player & player, uint32_t count);
      void Right(Ui::Player & player, uint32_t count);
      void Confirm();
      void Redraw();
      uint32_t Current() const;

   public:
      void AdjustScroll(Mpc::List list);

   public:
      std::string SearchPattern(uint32_t id) const
      {
         if (id < lists_->Size())
         {
            return lists_->Get(id).name_;
         }

         return "";
      }

   public:
      void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void AddAllLines();
      void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void DeleteAllLines();
      void CropLine(uint32_t line, uint32_t count, bool scroll);
      void CropAllLines();
      void Edit();
      void ScrollToFirstMatch(std::string const & input);

   protected:
      Main::WindowBuffer const & WindowBuffer() const { return *lists_; }

   private:
      void     SoftRedraw();
      void     Clear();
      uint32_t BufferSize() const { return lists_->Size(); }
      int32_t  DetermineColour(uint32_t line) const;

   private:
      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Ui::Search     const & search_;
      Mpc::Lists           * lists_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
