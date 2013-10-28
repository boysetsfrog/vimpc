/*
   Vimpc
   Copyright (C) 2010 - 2013 Nathan Sweetman

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

   nowplayingwindow.hpp - class representing a window that can scroll
   */

#ifndef __UI__NOWPLAYINGWINDOW
#define __UI__NOWPLAYINGWINDOW

#include "scrollwindow.hpp"

namespace Ui
{
   class Screen;

   class NowPlayingWindow : public ScrollWindow
   {
   public:
      NowPlayingWindow(Main::Settings const & settings, Ui::Screen & screen, std::string name = "nowplaying");
      virtual ~NowPlayingWindow();

   public:
      void Print(uint32_t line) const;
      uint32_t Current() const              { return CurrentLine(); };

      void Redraw();

   private:
      Main::WindowBuffer const & WindowBuffer() const { return buffer_; }

   private:
   Main::Buffer<std::string> buffer_;

   };
}

#endif
/* vim: set sw=3 ts=3: */
