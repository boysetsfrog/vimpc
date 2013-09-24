
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

   modewindow.hpp - provides a window for each input mode
   */

#ifndef __UI__PAGERWINDOW
#define __UI__PAGERWINDOW

#include "window.hpp"

#include "buffer/linebuffer.hpp"

#include <stdint.h>
#include <string>

namespace Ui
{
   class Screen;

   class PagerWindow : public Ui::Window
   {
   public:
      PagerWindow(Ui::Screen & screen, int columns, int lines);
      virtual ~PagerWindow();

   public:
      void AddLine(std::string const & line);
      void AddLine(char const * const fmt, ... );
      void Print(uint32_t line) const;
      void Clear();

      void Page();
      bool IsAtEnd();

      uint32_t BufferSize() const;

   private:
      Main::Buffer<std::string> buffer_;
      uint32_t                  currentLine_;
      Ui::Screen &              screen_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
