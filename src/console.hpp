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

   console.hpp - window to accept command mode input 
   */

#ifndef __UI__CONSOLE
#define __UI__CONSOLE

#include "window.hpp"

#include <string>

namespace Ui
{
   class ConsoleWindow : public Ui::Window
   {
   public:
      ConsoleWindow(Ui::Screen const & screen);
      ~ConsoleWindow();

   public:
      void Print(uint32_t line) const;

   public:
      void OutputLine(char const * const fmt, ...);
      void Clear();

   private:
      size_t BufferSize() const { return buffer_.size(); }

   private:
      typedef std::vector<std::string> WindowBuffer;
      WindowBuffer buffer_;
   };
}

#endif
