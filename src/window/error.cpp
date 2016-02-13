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

   error.cpp - modewindow used to display an error
   */

#include "error.hpp"


void Error(uint32_t errorNumber, std::string errorString)
{
   if ((errorNumber != 0) && (errorNumber < (static_cast<uint32_t>(ErrorNumber::ErrorCount))))
   {
      Ui::ErrorWindow & errorWindow(Ui::ErrorWindow::Instance());

      errorWindow.ErrorMutex.lock();

		if (errorNumber == ErrorNumber::ErrorClear) 
		{
			errorWindow.ClearError();
	   } 
		else if (errorWindow.HasError() == false)
      {
         errorWindow.SetError(true);
         errorWindow.SetLine("E%d: %s", errorNumber, errorString.c_str());
      }

      errorWindow.ErrorMutex.unlock();
   }
}

void ErrorString(uint32_t errorNumber)
{
   if ((errorNumber != 0) && (errorNumber < (static_cast<uint32_t>(ErrorNumber::ErrorCount))))
   {
      Error(errorNumber, ErrorStrings::Default[errorNumber]);
      Debug("ERROR E%d: %s", errorNumber, ErrorStrings::Default[errorNumber].c_str());
   }
}

void ErrorString(uint32_t errorNumber, std::string additional)
{
   if ((errorNumber != 0) && (errorNumber < (static_cast<uint32_t>(ErrorNumber::ErrorCount))))
   {
      Error(errorNumber, ErrorStrings::Default[errorNumber] + ": " + additional);
      Debug("ERROR E%d: %s", errorNumber, std::string(ErrorStrings::Default[errorNumber] + ": " + additional).c_str());
   }
}

/* vim: set sw=3 ts=3: */
