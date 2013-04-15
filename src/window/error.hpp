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

#include "errorcodes.hpp"
#include "settings.hpp"
#include "modewindow.hpp"

#include <stdint.h>
#include <string>

//! Display an error window with the given error
static void Error(uint32_t errorNumber, std::string errorString);
static void ErrorString(uint32_t errorNumber);
static void ErrorString(uint32_t errorNumber, std::string additional);

// Errors cannot be added to the window directly
// The Accessor functions defined above must be used
namespace Ui
{
   class ErrorWindow : private Ui::ModeWindow
   {
      friend class Ui::Screen;
      friend void ::Error(uint32_t errorNumber, std::string errorString);

   // Everything is private to prevent errors being set on the window
   // directly without using the helper functions
   private:
      static ErrorWindow & Instance()
      {
         static ErrorWindow errorWindow;
         return errorWindow;
      }

      ErrorWindow() : ModeWindow(COLS, LINES), hasError_(false) { }
      ~ErrorWindow() { }

   private:
      void Print(uint32_t line) const
      {
         if (Main::Settings::Instance().Get(Setting::ColourEnabled) == true)
         {
            wattron(N_WINDOW(), COLOR_PAIR(Main::Settings::Instance().colours.Error) | A_BOLD);
         }

         ModeWindow::Print(line);

         if (Main::Settings::Instance().Get(Setting::ColourEnabled) == true)
         {
            wattroff(N_WINDOW(), COLOR_PAIR(Main::Settings::Instance().colours.Error) | A_BOLD);
         }
      }

      void ClearError()             { hasError_ = false; }
      bool HasError() const         { return hasError_; }
      void SetError(bool hasError)  { hasError_ = hasError; }

   private:
      bool hasError_;
   };
}

void Error(uint32_t errorNumber, std::string errorString)
{
   if ((errorNumber != 0) && (errorNumber < (static_cast<uint32_t>(ErrorNumber::ErrorCount))))
   {
      Ui::ErrorWindow & errorWindow(Ui::ErrorWindow::Instance());
      errorWindow.SetError(true);
      errorWindow.SetLine("E%d: %s", errorNumber, errorString.c_str());
   }
}

void ErrorString(uint32_t errorNumber)
{
   if ((errorNumber != 0) && (errorNumber < (static_cast<uint32_t>(ErrorNumber::ErrorCount))))
   {
      Error(errorNumber, ErrorStrings::Default[errorNumber]);
   }
}

void ErrorString(uint32_t errorNumber, std::string additional)
{
   if ((errorNumber != 0) && (errorNumber < (static_cast<uint32_t>(ErrorNumber::ErrorCount))))
   {
      Error(errorNumber, ErrorStrings::Default[errorNumber] + ": " + additional);
   }
}

#endif
/* vim: set sw=3 ts=3: */
