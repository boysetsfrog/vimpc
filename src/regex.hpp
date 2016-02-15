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

#include <string>

#include <pcre.h>
#include <string.h>

#ifndef __REGEX__RE
#define __REGEX__RE

namespace Regex
{
   typedef enum
   {
      None            = 0,
      CaseInsensitive = PCRE_CASELESS,
      UTF8            = PCRE_UTF8
   } Options;

   class RE
   {
      public:
         RE(std::string exp);
         RE(std::string exp, Regex::Options opt);
         ~RE();

      public:
			static void Trim(std::string & input);

		public:
         inline bool Matches(std::string match) const
         {
            Compile(exp_);
            return IsMatch(match);
         }

         inline bool CompleteMatch(std::string match) const
         {
            Compile("(?:" + exp_ + ")\\z");
            return IsMatch(match);
         }


         bool Capture(std::string match,
                      std::string * arg1,        std::string * arg2 = NULL,
                      std::string * arg3 = NULL, std::string * arg4 = NULL,
                      std::string * arg5 = NULL, std::string * arg6 = NULL,
                      std::string * arg7 = NULL, std::string * arg8 = NULL) const;

         bool Replace(std::string substitution, std::string & valueString) const;

			inline bool ReplaceAll(std::string substitution, std::string & valueString) const
			{
				bool replaced = false;

				while (Replace(substitution, valueString)) 
				{
					replaced = true;
				}

				return replaced;
			}

      private:
         void Compile(std::string toCompile) const;
         bool IsMatch(std::string match) const;
         void ErrorPrint(int rc) const;
         void Free() const;

      private:
         std::string const   exp_;
         mutable std::string compiled_;
         mutable pcre *      re_;
         Regex::Options const opt_;
   };
}

#endif
