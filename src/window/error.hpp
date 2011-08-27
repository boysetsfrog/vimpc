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

#include "colour.hpp"
#include "errorcodes.hpp"
#include "modewindow.hpp"

#include <stdint.h>
#include <string>

namespace Ui
{
   class ErrorWindow : public Ui::ModeWindow
   {
   //! \todo why is this a singleton?
   public:
      static ErrorWindow & Instance()
      {
         static ErrorWindow errorWindow;
         return errorWindow;
      }

   private:
      ErrorWindow() : ModeWindow(), hasError_(false) { }
      ~ErrorWindow() { }

   public:
      void Print(uint32_t line) const
      {
         wattron(N_WINDOW(), COLOR_PAIR(Colour::Error) | A_BOLD);
         ModeWindow::Print(line);
         wattroff(N_WINDOW(), COLOR_PAIR(Colour::Error) | A_BOLD);
      }

      void ClearError()             { hasError_ = false; }
      void SetError(bool hasError)  { hasError_ = hasError; }
      bool HasError() const         { return hasError_; }

   private:
      bool hasError_;
   };
}

static void Error(uint32_t errorNumber, std::string errorString);

void Error(uint32_t errorNumber, std::string errorString)
{
   if (errorNumber != 0)
   {
      Ui::ErrorWindow & errorWindow(Ui::ErrorWindow::Instance());
      errorWindow.SetError(true);
      errorWindow.SetLine("E%d: %s", errorNumber, errorString.c_str());
   }
   //\todo if a critical error, also print to console window
}

#endif
