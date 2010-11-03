#ifndef __MAIN__CONFIG
#define __MAIN__CONFIG

#include <fstream>
#include <stdlib.h>

namespace Main
{
   namespace Config
   {
      static bool ExecuteConfigCommands(Ui::Commands & handler);
   }
}

bool Main::Config::ExecuteConfigCommands(Ui::Commands & handler)
{
   static char const * const vimpcrcFile = "/.vimpcrc";
   static char const * const homeEnv     = "HOME";
   static bool configCommandsExecuted    = false;

   bool result = false;

   if (configCommandsExecuted == false)
   {
      configCommandsExecuted = true;

      std::string configFile(getenv(homeEnv));
      configFile.append(vimpcrcFile);
      
      std::string   input;
      std::ifstream inputStream(configFile.c_str());

      while (!inputStream.eof())
      {
         std::getline(inputStream, input);
         result = handler.ExecuteCommand(input);
      }
   }

   return result;
}
#endif
