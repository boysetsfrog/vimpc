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

   config.hpp - parses vimpcrc file and executes commands
   */

#ifndef __MAIN__CONFIG
#define __MAIN__CONFIG

#include <fstream>
#include <stdlib.h>

#include "regex.hpp"

namespace Main
{
   namespace Config
   {
      static bool ExecuteConfigCommands(Ui::Command & handler);
   }
}


bool fexists(const std::string& filename) {
   std::ifstream ifile(filename.c_str());
	return (bool)ifile;
}

bool Main::Config::ExecuteConfigCommands(Ui::Command & handler)
{
   static bool configCommandsExecuted    = false;

   Regex::RE const commentCheck("^\\s*\".*");

   if (configCommandsExecuted == false)
   {
      configCommandsExecuted = true;

      static char const * const home_dir = getenv("HOME");
      static char const * const xdg_config_dir = getenv("XDG_CONFIG_HOME");
      std::string configFile;
      std::string xdgPath;

      if (xdg_config_dir)
      {
         xdgPath = std::string(xdg_config_dir).append("/vimpc/vimpcrc");
      }
      std::string configPath = std::string(home_dir).append("/.config/vimpc/vimpcrc");
      std::string fallback = std::string(home_dir).append("/.vimpcrc");
      if (xdg_config_dir != NULL && fexists(xdgPath)) {
         configFile = xdgPath;
      }
      else if (fexists(configPath))
      {
         configFile = configPath;
      }
      else if (fexists(fallback))
      {
         configFile = fallback;
      }

      std::ifstream inputStream(configFile);
      std::string input;

      if (inputStream)
      {
         while (!inputStream.eof())
         {
            std::getline(inputStream, input);

            if ((input != "") && (commentCheck.Matches(input.c_str()) == false))
            {
               handler.ExecuteCommand(input);
            }
         }
      }
   }

   return true;
}
#endif
/* vim: set sw=3 ts=3: */
