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

#ifndef __UI__MODEWINDOW
#define __UI__MODEWINDOW

#include "window.hpp"
#include "buffer/linebuffer.hpp"

#include <stdint.h>
#include <string>

namespace Ui
{
   class Screen;

   class ModeWindow : public Ui::Window
   {
      friend class Ui::Screen;

   public:
      ModeWindow(int columns, int lines);
      virtual ~ModeWindow();

   public:
      void SetLine(std::string const & line);
      void SetLine(char const * const fmt, ... );

      uint32_t BufferSize() const { return buffer_.Size(); }
      void SetCursorPosition(uint32_t cursorPosition);
      void ShowCursor();
      void HideCursor();

   protected:
      virtual void Print(uint32_t line) const;

   private:
      bool             cursorVisible_;
      uint32_t         cursorPosition_;
      Main::LineBuffer buffer_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
