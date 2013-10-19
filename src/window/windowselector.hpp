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

   windowselector.hpp - window to select other windows
   */

#ifndef __UI__WINDOWSELECTOR
#define __UI__WINDOWSELECTOR

#include <string>

#include "buffers.hpp"
#include "buffer/buffer.hpp"
#include "mode/search.hpp"
#include "window/selectwindow.hpp"

namespace Ui
{

   //
   class WindowSelector : public Ui::SelectWindow
   {
   public:
      WindowSelector(Main::Settings const & settings, Ui::Screen & screen,
                     Ui::Windows const & windows, Ui::Search const & search);
      ~WindowSelector();

   public:
      void Clear();
      void Confirm();

      uint32_t Current() const { return ScrollLine(); }

 public:
      void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void AddAllLines();
      void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void DeleteAllLines();

   protected:
      int32_t DetermineColour(uint32_t line) const;

      Main::WindowBuffer const & WindowBuffer() const { return windows_; }

   private:
      Ui::Windows const & windows_;
      Main::Settings const & settings_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
