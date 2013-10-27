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

   help.hpp - help window
   */

#ifndef __UI__HELP
#define __UI__HELP

#include "buffer/buffer.hpp"
#include "window/selectwindow.hpp"

namespace Main { class Settings; }
namespace Ui   { class Search; }

namespace Ui
{
   class HelpWindow : public Ui::SelectWindow
   {
   public:
      HelpWindow(Main::Settings const & settings, Ui::Screen & screen, Ui::Search const & search);
      ~HelpWindow();

   private:
      void Print(uint32_t line) const;

   public:
      void Redraw();
      void Confirm();
      void Scroll(int32_t scrollCount);
      void ScrollTo(uint32_t scrollLine);

      std::string SearchPattern(uint32_t id) const { return help_.Get(id); }

   protected:
      Main::WindowBuffer const & WindowBuffer() const { return help_; }

   private:
      void Clear();
      void LoadHelpFile();

   private:
      Main::Settings const & settings_;
      Ui::Search     const & search_;
      typedef Main::Buffer<std::string> HelpBuffer;
      HelpBuffer help_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
