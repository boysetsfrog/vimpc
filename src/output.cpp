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

   output.cpp - represents information of an available output
   */

#include "output.hpp"

#include <stdio.h>

using namespace Mpc;

Output::Output(uint32_t id) :
   id_       (id),
   name_     (""),
   enabled_  (false)
{ }

Output::~Output()
{ }


uint32_t Output::Id() const
{
   return id_;
}

void Output::SetName(const char * name)
{
   if (name != NULL)
   {
      name_ = name;
   }
   else
   {
      name_ = "Unknown";
   }
}

std::string const & Output::Name() const
{
   return name_;
}

void Output::SetEnabled(bool enable)
{
   enabled_ = enable;
}

bool Output::Enabled() const
{
   return enabled_;
}

std::string Output::PrintString() const
{
   return "$H[" + std::string(enabled_ ? "$I*$D" : " ") + std::string("] $H") + name_;
}

/* vim: set sw=3 ts=3: */
