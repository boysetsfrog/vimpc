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

   help.hpp - help window 
   */

#ifndef __UI__HELP
#define __UI__HELP

#include "scrollwindow.hpp"

namespace Ui
{
   class HelpWindow : public Ui::ScrollWindow
   {
   public:
      HelpWindow(Ui::Screen const & screen);
      ~HelpWindow();

   public:
      void Redraw();
      void Print(uint32_t line) const;
      void Confirm();

      //! \todo work out why this doesn't work properly
      std::string SearchPattern(int32_t id) { return buffer_.at(id); }

      uint16_t CurrentLine() const { return 0; }

   private:
      void Clear();
      void LoadHelpFile();

   private:
      size_t BufferSize() const { return buffer_.size(); }

   private:
      typedef std::vector<std::string> HelpBuffer;
      HelpBuffer buffer_;
   };
}

#endif
