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
#include "events.hpp"
#include "output.hpp"
#include "vimpc.hpp"

// Outputs
namespace Mpc
{
   class Outputs : public Main::Buffer<Mpc::Output *>
   {
   public:
      Outputs()
      {
         Main::Vimpc::EventHandler(Event::OutputEnabled,  [this] (EventData const & Data)
            { Debug("OutputEnabled %d", Data.id); SetOutput(Data.id, true); });
         Main::Vimpc::EventHandler(Event::OutputDisabled, [this] (EventData const & Data)
            { Debug("OutputDisabled %d", Data.id); SetOutput(Data.id, false); });
      }
      ~Outputs()
      {
      }

      void SetOutput(uint32_t id, bool enabled)
      {
         for (unsigned int i = 0; i < Size(); ++i)
         {
            if (Get(i)->Id() == id)
            {
               Get(i)->SetEnabled(enabled);
            }
         }
      }

      std::string String(uint32_t position) const      { return Get(position)->PrintString(); }
      std::string PrintString(uint32_t position) const { return Get(position)->PrintString(); }
   };
}
#endif
/* vim: set sw=3 ts=3: */
