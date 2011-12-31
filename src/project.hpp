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

   project.hpp - information about the current project
   */

#ifndef __MAIN__PROJECT
#define __MAIN__PROJECT

#include "config.h"

#include <sstream>

#ifndef PACKAGE_URL
#define PACKAGE_URL ""
#endif

#ifndef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT ""
#endif

namespace Main
{
   namespace Project
   {
      static std::string const & BugReport()
      {
         static std::string const bugReport(PACKAGE_BUGREPORT);
         return bugReport;
      }

      static std::string const & URL()
      {
         static std::string const url(PACKAGE_URL);
         return url;
      }

      static std::string const & Version()
      {
         static std::ostringstream versionBuffer;

      #ifdef PACKAGE_GIT_REVISION
         versionBuffer << PACKAGE_STRING << " [" << PACKAGE_GIT_REVISION << "]" ;
      #else
      # ifdef PACKAGE_SVN_REVISION
         versionBuffer << PACKAGE_STRING << " [" << PACKAGE_SVN_REVISION << "]" ;
      # else
         versionBuffer << PACKAGE_STRING;
      # endif
      #endif

         static std::string const VersionString(versionBuffer.str());
         return VersionString;
      }
   }
}

#endif
/* vim: set sw=3 ts=3: */
