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

   mode.hpp - abstract class for all modes
   */

#ifndef __UI__MODE
#define __UI__MODE

namespace Ui
{
   class Mode
   {
   public:
      virtual ~Mode() { }

   public:
      //! Called whenever the mode is initialised
      //!
      //! \param input The input that caused the mode change
      virtual void Initialise(int input) = 0;

      //! Called whenever a mode is going to be canceled/completed
      //!
      //! \param input The input that caused the mode change
      virtual void Finalise(int input) = 0;

      //! Called when the screen requires refreshing, which
      //! may require a change to the mode
      virtual void Refresh() = 0;

      //! Handles the given input
      //!
      //! \param input Single character input to handle
      virtual bool Handle(int input) = 0;

      //! Determines whether the given input will cause a switch
      //! to this mode
      //!
      //! \param input input to check for a mode change
      virtual bool CausesModeToStart(int input) const = 0;
      virtual bool CausesModeToEnd(int input) const = 0;
   };
}

#endif
/* vim: set sw=3 ts=3: */
