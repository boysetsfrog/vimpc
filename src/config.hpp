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

   config.hpp - parses .vimpcrc file and executes commands 
   */

#ifndef __MAIN__CONFIG
#define __MAIN__CONFIG

#include <fstream>
#include <stdlib.h>

namespace Main
{
   namespace Config
   {
      static bool ExecuteConfigCommands(Ui::Command & handler);
   }
}

bool Main::Config::ExecuteConfigCommands(Ui::Command & handler)
{
   static char const * const vimpcrcFile = "/.vimpcrc";
   static char const * const home        = "HOME";
   static bool configCommandsExecuted    = false;

   bool result = false;

   if (configCommandsExecuted == false)
   {
      configCommandsExecuted = true;

      std::string configFile(getenv(home));
      configFile.append(vimpcrcFile);
      
      std::string   input;
      std::ifstream inputStream(configFile.c_str());

      while (!inputStream.eof())
      {
         std::getline(inputStream, input);
         
         if (input != "")
         {
            result = handler.ExecuteCommand(input);
         }
      }
   }

   return result;
}
#endif
