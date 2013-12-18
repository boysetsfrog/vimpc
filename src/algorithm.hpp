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

   algorithm.hpp -
   */

#ifndef __MAIN__ALGORITHM
#define __MAIN__ALGORITHM

#include <algorithm>

#include <string>

#include "regex.hpp"

namespace Algorithm
{
   template <class T>
   bool isCase(std::string const & s1, T op);
   bool isLower(std::string const & s1);
   bool isUpper(std::string const & s1);
   bool icompare(std::string const & s1, std::string const & s2, bool ignoreLeadingThe = false, bool caseInsensitive = true);
   bool imatch(std::string const & s1, std::string const & s2, bool ignoreLeadingThe, bool caseInsensitive);
   bool iequals(std::string const & s1, std::string const & s2);
   bool iequals(std::string const & s1, std::string const & s2, bool ignoreLeadingThe, bool caseInsensitive);

   bool isNumeric(std::string const & s1);
}


template <class T>
bool Algorithm::isCase(std::string const & s1, T op)
{
   std::string case1(s1);
   std::transform(case1.begin(), case1.end(), case1.begin(), op);
   return (s1 == case1);
}

#endif

/* vim: set sw=3 ts=3: */
