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

   linebuffer.hpp -
   */

#ifndef __MAIN__LINEBUFFER
#define __MAIN__LINEBUFFER

#include <string>

#include "buffer/buffer.hpp"

namespace Main
{
   class LineBuffer : public Buffer<std::string>
   {
   public:
      LineBuffer() { }
      ~LineBuffer() { }

   public:
      std::string const & Get(uint32_t position) const
      {
         return Buffer<LineBuffer::BufferType>::Get(0);
      }

      void Add(std::string entry)
      {
         Buffer<LineBuffer::BufferType>::Clear();
         Buffer<LineBuffer::BufferType>::Add(entry);
      }
   };
}
#endif
/* vim: set sw=3 ts=3: */
