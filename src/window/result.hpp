/*
   Vimpc
   Copyright (C) 2012 Nathan Sweetman

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

   result.hpp - modewindow used to display a single line result
   */

#ifndef __UI__RESULT
#define __UI__RESULT

#include "colour.hpp"
#include "errorcodes.hpp"
#include "settings.hpp"
#include "modewindow.hpp"

#include <stdint.h>
#include <string>

//! Display a result window with the given string
static void Result(std::string errorString);

// Errors cannot be added to the window directly
// The Accessor functions defined above must be used
namespace Ui
{
   class ResultWindow : private Ui::ModeWindow
   {
      friend class Ui::Screen;
      friend void ::Result(std::string errorString);

   // Everything is private to prevent errors being set on the window
   // directly without using the helper functions
   private:
      static ResultWindow & Instance()
      {
         static ResultWindow window;
         return window;
      }

      ResultWindow() : ModeWindow(COLS, LINES), hasResult_(false) { }
      ~ResultWindow() { }

   private:
      void ClearResult()              { hasResult_ = false; }
      bool HasResult() const          { return hasResult_; }
      void SetResult(bool hasResult)  { hasResult_ = hasResult; }

   private:
      bool hasResult_;
   };
}

void Result(std::string result)
{
   Ui::ResultWindow & window(Ui::ResultWindow::Instance());
   window.SetResult(true);
   window.SetLine("%s", result.c_str());
}

#endif
/* vim: set sw=3 ts=3: */
