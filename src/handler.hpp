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

   handler.hpp - abstract class for all modes 
   */

#ifndef __UI__HANDLER
#define __UI__HANDLER

namespace Ui
{
   class Handler
   {
   public:
      virtual ~Handler() { }

   public:
      virtual void InitialiseMode()  = 0;
      virtual void FinaliseMode()    = 0;
      virtual bool Handle(int input) = 0;
      virtual bool CausesModeToStart(int input) = 0;
   };
}

#endif
