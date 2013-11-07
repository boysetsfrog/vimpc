/*
   Vimpc
   Copyright (C) 2010 - 2013 Nathan Sweetman

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

   regex.hpp - C++ wrapper around pcre and/or other regex libraries
*/

#include <pcre.h>
#include <pcrecpp.h>

namespace Regex
{
   class RE
   {
      public:
        RE(std::string exp) :
            exp_    (exp),
            pcrecpp_(exp)
        {
        }

      public:
        bool FullMatch(std::string match) const
        {
            return pcrecpp_.FullMatch(match);
        }

      private:
         std::string const exp_;
         pcrecpp::RE const pcrecpp_;
   };
}
