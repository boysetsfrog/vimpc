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

   algorithm.cpp -
   */

#include "algorithm.hpp"

std::string PrepString(std::string const & s1, bool ignoreLeadingThe, bool caseInsensitive)
{
   std::string Result = s1;

   if (ignoreLeadingThe == true)
	{
      pcrecpp::RE("\\s*[tT][hH][eE]\\s*").Replace("", &Result);  
   }

	if (caseInsensitive == true)
	{
		std::transform(Result.begin(), Result.end(), Result.begin(), ::tolower);
	}

   return Result;
}

bool Algorithm::isLower(std::string const & s1)
{
   return isCase(s1, ::tolower);
}

bool Algorithm::isUpper(std::string const & s1)
{
   return isCase(s1, ::toupper);
}

bool Algorithm::icompare(std::string const & s1, std::string const & s2, bool ignoreLeadingThe, bool caseInsensitive)
{
   std::string const lower1(PrepString(s1, ignoreLeadingThe, caseInsensitive));
   std::string const lower2(PrepString(s2, ignoreLeadingThe, caseInsensitive));
   return (lower1 < lower2);
}

bool Algorithm::imatch(std::string const & s1, std::string const & s2, bool ignoreLeadingThe, bool caseInsensitive)
{
   std::string lower1(PrepString(s1, ignoreLeadingThe, caseInsensitive));
   std::string lower2(PrepString(s2, ignoreLeadingThe, caseInsensitive));

   if (lower1.size() > lower2.size())
   {
      lower1 = lower1.substr(0, lower2.length());
   }
   else if (lower2.size() > lower1.size())
   {
      lower2 = lower2.substr(0, lower1.length());
   }
   return (lower1 == lower2);
}


bool Algorithm::iequals(std::string const & s1, std::string const & s2)
{
   std::string lower1(s1);
   std::string lower2(s2);

   std::transform(lower1.begin(), lower1.end(), lower1.begin(), ::tolower);
   std::transform(lower2.begin(), lower2.end(), lower2.begin(), ::tolower);

   return (lower1 == lower2);
}

bool Algorithm::isNumeric(std::string const & s1)
{
   for (unsigned int i = 0; i < s1.size(); ++i)
   {
      if ((s1[i] > '9') || (s1[i] < '0'))
      {
         return false;
      }
   }

   return true;
}

/* vim: set sw=3 ts=3: */
