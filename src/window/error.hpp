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

   error.hpp - modewindow used to display an error
   */

#ifndef __UI__ERROR
#define __UI__ERROR

#include "compiler.hpp"

#include "errorcodes.hpp"
#include "settings.hpp"
#include "modewindow.hpp"
#include "test.hpp"
#include "window/debug.hpp"

#include <stdint.h>
#include <string>
#include <stdarg.h>

//! Display an error window with the given error
void Error(uint32_t errorNumber, std::string errorString);
void ErrorString(uint32_t errorNumber);
void ErrorString(uint32_t errorNumber, std::string additional);

// Errors cannot be added to the window directly
// The Accessor functions defined above must be used
namespace Ui
{
   class ErrorWindow : private Ui::ModeWindow
   {
      friend class Ui::Screen;
      friend void ::Error(uint32_t errorNumber, std::string errorString);

   // Everything is protected to prevent errors being set on the window
   // directly without using the helper functions, but allow test
   // functions access
   protected:
      static ErrorWindow & Instance()
      {
         static ErrorWindow errorWindow;
         return errorWindow;
      }

      ErrorWindow() : ModeWindow(COLS, LINES), hasError_(false) { }
      ~ErrorWindow() { }

      void Print(uint32_t line) const
      {
         ErrorMutex.lock();

         if (Main::Settings::Instance().Get(Setting::ColourEnabled) == true)
         {
            wattron(N_WINDOW(), COLOR_PAIR(Main::Settings::Instance().colours.Error) | A_BOLD);
         }

         ModeWindow::Print(line);

         if (Main::Settings::Instance().Get(Setting::ColourEnabled) == true)
         {
            wattroff(N_WINDOW(), COLOR_PAIR(Main::Settings::Instance().colours.Error) | A_BOLD);
         }

         ErrorMutex.unlock();
      }

      void ClearError() { SetError(false); }

      bool HasError() const
      {
         ErrorMutex.lock();
         bool const hasError = hasError_;
         ErrorMutex.unlock();
         return hasError;
      }

      void SetError(bool hasError)
      {
         ErrorMutex.lock();
         hasError_ = hasError;
         ErrorMutex.unlock();
      }

   private:
      bool                   hasError_;
      mutable RecursiveMutex ErrorMutex;
   };
}

#endif
/* vim: set sw=3 ts=3: */
