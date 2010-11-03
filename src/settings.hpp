#ifndef __MAIN__SETTINGS
#define __MAIN__SETTINGS

#include "screen.hpp"

#include <string>
#include <map>

namespace Main
{
   class Settings
   {
      public:
         static Settings & Instance();

      protected:
         Settings();
         ~Settings();

      public:
         //Calls the correct setter function based upon the given input
         void Set(std::string const & input);

         //Sets or Gets the default startup window
         Ui::Screen::MainWindow Window() const;
         void SetWindow(std::string const & arguments); 

      private:
         Ui::Screen::MainWindow defaultWindow_;

         typedef void (Main::Settings::*ptrToMember)(std::string const &);
         typedef std::map<std::string, ptrToMember> SettingsTable;
         SettingsTable settingsTable_;

         typedef std::map<std::string, Ui::Screen::MainWindow> WindowTable;
         WindowTable windowTable_;
   };
}

#endif
