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

#include <map>

#include "regex.hpp"
#include "window/debug.hpp"

using namespace Regex;

RE::RE(std::string exp) :
    exp_     (exp),
    compiled_(""),
    opt_     (Regex::None),
    re_      (NULL)
{
}

RE::RE(std::string exp, Regex::Options opt) :
    exp_     (exp),
    compiled_(""),
    opt_     (opt),
    re_      (NULL)
{
}

RE::~RE()
{
   Free();
}


/* static */ void RE::Trim(std::string & input)
{
	Regex::RE start("^\\s+");
	Regex::RE end("\\s+$");

	start.Replace("", input);
	end.Replace("", input);
}

void RE::Compile(std::string toCompile) const
{
   char const * error;
   int erroffset;

   if (toCompile != compiled_)
   {
       Free();

       if (re_ == NULL)
       {
           Debug("Doing PCRE compilation on %s", exp_.c_str());
           re_ = pcre_compile(toCompile.c_str(), opt_, &error, &erroffset, NULL);

           if (re_ == NULL)
           {
              Debug("PCRE compilation failed at offset %d: %s\n", erroffset, error);
           }
           else
           {
              compiled_ = toCompile;
           }
       }
   }
}

bool RE::Capture(std::string match,
                 std::string * arg1, std::string * arg2,
                 std::string * arg3, std::string * arg4,
                 std::string * arg5, std::string * arg6,
                 std::string * arg7, std::string * arg8) const
{
   Compile(exp_);

   std::map<int, std::string *> args;

   args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4;
   args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8;

   int const vecsize = 24;
   int ovector[vecsize];

   int rc = pcre_exec(re_, NULL, match.c_str(), match.length(), 0, 0, ovector, vecsize);

   ErrorPrint(rc);

   for (int i = 0; i < 7; ++i)
   {
      if (args[i] != NULL)
      {
         *args[i] = "";
      }
   }

   if (rc == 0)
   {
      rc = vecsize / 3;
   }

   if (rc >= 1)
   {
      --rc;

      for (int i = 0; ((i < rc) && (2*i < vecsize)); ++i)
      {
         if (args[i] != NULL)
         {
            int const off = 2 * (i + 1);
            *args[i] = match.substr(ovector[off], ovector[off + 1] - ovector[off]);
            Debug("Captured %s", args[i]->c_str());
         }
      }
   }

   return (rc >= 1);
}


bool RE::Replace(std::string substitution, std::string & valueString) const
{
   Compile(exp_);

   int const vecsize = 24;
   int ovector[vecsize];

   int rc = pcre_exec(re_, NULL, valueString.c_str(), valueString.length(), 0, 0, ovector, vecsize);

   ErrorPrint(rc);

   if (rc == 0)
   {
      rc = vecsize / 3;
   }

   if (rc >= 1)
   {
      --rc;
      int const off = 0;
      Debug("Replacing %s in %s with %s", exp_.c_str(), valueString.c_str(), substitution.c_str());
      valueString.replace(ovector[off], ovector[off + 1] - ovector[off], substitution);
      return true;
   }

   return false;
}

bool RE::IsMatch(std::string match) const
{
   int const vecsize = 12;
   int ovector[vecsize];

   int const rc = pcre_exec(re_, NULL, match.c_str(), match.length(), 0, 0, ovector, vecsize);

   ErrorPrint(rc);
   return (rc >= 0);
}

void RE::ErrorPrint(int rc) const
{
   if (rc <= 0)
   {
       if (rc == 0)
       {
           Debug("Regex ovector is insufficient\n");
       }
       else if (rc != PCRE_ERROR_NOMATCH)
       {
           Debug("Regex rc error %d\n", rc);
       }
   }
}

void RE::Free() const
{
    if (re_ != NULL)
    {
        compiled_ = "";
        pcre_free(re_);
        re_ = NULL;
    }
}
