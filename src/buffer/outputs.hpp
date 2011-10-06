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

   outputs.hpp - handling of outputs
   */

#ifndef __MPC__OUTPUTS
#define __MPC__OUTPUTS

// Includes
#include "buffer.hpp"
#include "callback.hpp"
#include "output.hpp"

// Outputs
namespace Mpc
{
   class Outputs : public Main::Buffer<Mpc::Output *>
   {
   private:
      typedef Main::CallbackObject<Mpc::Outputs, Outputs::BufferType> CallbackObject;
      typedef Main::CallbackFunction<Outputs::BufferType> CallbackFunction;

   public:
      Outputs()
      {
      }
      ~Outputs()
      {
      }
   };
}
#endif
