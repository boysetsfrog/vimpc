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

   output.hpp - represents information of an available output
   */

#ifndef __MPC_OUTPUT
#define __MPC_OUTPUT

#include <stdint.h>
#include <string>

namespace Mpc
{
   class Output
   {
   public:
      Output(uint32_t id);
      ~Output();

   private:
      Output(Output const & output);

   public:
      bool operator<(Output const & rhs) const
      {
         return (name_ < rhs.name_);
      }

   public:
      uint32_t Id() const;

      void SetName(const char * name);
      std::string const & Name() const;

      void SetEnabled(bool enable);
      bool Enabled() const;

      std::string PrintString() const;

   private:
      uint32_t    id_;
      std::string name_;
      bool        enabled_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
