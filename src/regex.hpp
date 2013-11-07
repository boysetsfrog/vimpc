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
            re_     (NULL)
        {
        }

        ~RE()
        {
            if (re_ != NULL)
            {
                pcre_free(re_);
                re_ = NULL;
            }
        }

      public:
        bool Matches(std::string match) const
        {
            Compile();
            int ovector[30];

            int rc = pcre_exec(re_, NULL, match.c_str(), match.length(), 0, 0, ovector, 30);

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

                return false;
            }

            return (rc >= 1);
        }

      private:
         void Compile() const
         {
            const char *error;
            int erroffset;

            if (re_ == NULL)
            {
                Debug("Doing PCRE compilation on %s", exp_.c_str());
                re_ = pcre_compile(exp_.c_str(), 0, &error, &erroffset, NULL);
            }

            if (re_ == NULL)
            {
                Debug("PCRE compilation failed at offset %d: %s\n", erroffset, error);
            }
         }

      private:
         std::string const exp_;
         mutable pcre *    re_;
   };
}
