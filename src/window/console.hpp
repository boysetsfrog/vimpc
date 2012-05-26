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

   console.hpp - window to display debug information on a console like screen
   */

#ifndef __UI__CONSOLE
#define __UI__CONSOLE

#include <string>

#include "buffer/buffer.hpp"
#include "window/scrollwindow.hpp"

//! \todo seperate console from console window
namespace Ui
{
   class Console : public Main::Buffer<std::string>
   {
   public:
      Console()  { }
      ~Console() { }
   };

   //
   class ConsoleWindow : public Ui::ScrollWindow
   {
   public:
      ConsoleWindow(Ui::Screen & screen, std::string name, Console & console);
      ~ConsoleWindow();

   public:
      void Print(uint32_t line) const;
      void Clear();
      void PerformAutoScroll(Console::BufferType string);

      uint32_t Current() const { return ScrollLine(); }

   private:
      typedef Main::CallbackObject<Ui::ConsoleWindow, Console::BufferType> CallbackObject;

   private: //Ui::Window
      size_t BufferSize() const { return console_.Size(); }

   private:
      Console & console_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
