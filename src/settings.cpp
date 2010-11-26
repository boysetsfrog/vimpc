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

   settings.cpp - handle configuration options via :set command
   */

#include "settings.hpp"

#include "assert.hpp"
#include "error.hpp"

#include <algorithm>
#include <boost/regex.hpp>
#include <iostream>
#include <sstream>
#include <stdlib.h>

using namespace Main;

char const * const AutoScrollSetting      = "autoscroll";
char const * const StopOnQuitSetting      = "stoponquit";
char const * const HighlightSearchSetting = "hlsearch";

Settings & Settings::Instance()
{
   static Settings settings_;
   return settings_; 
}

Settings::Settings() :
   defaultWindow_(Ui::Screen::Console)
{
   settingsTable_["window"]             = &Settings::SetWindow;

   toggleTable_[AutoScrollSetting]      = new Setting<bool>(false);
   toggleTable_[StopOnQuitSetting]      = new Setting<bool>(true);
   toggleTable_[HighlightSearchSetting] = new Setting<bool>(true);
}

Settings::~Settings()
{
}


void Settings::Set(std::string const & input)
{
   std::string       setting, arguments;
   std::stringstream settingStream(input);

   std::getline(settingStream, setting,   ' '); 
   std::getline(settingStream, arguments, '\n');

   if (arguments == "")
   {
      SetSingleSetting(setting);
   }
   else
   {
      SetSpecificSetting(setting, arguments);
   }

}

void Settings::SetSpecificSetting(std::string setting, std::string arguments)
{
   if (settingsTable_.find(setting) != settingsTable_.end())
   {
      ptrToMember settingFunc = settingsTable_[setting];
      (*this.*settingFunc)(arguments);
   }
   else
   {
      Error(1, "No such setting: " + setting);
   }
}

void Settings::SetSingleSetting(std::string setting)
{
   boost::regex const toggleCheck("^.*!$");
   boost::regex const offCheck   ("^no.*");

   bool const toggle(boost::regex_match(setting.c_str(), toggleCheck));
   bool const off   (boost::regex_match(setting.c_str(), offCheck));

   if (toggle == true)
   {
      setting = setting.substr(0, setting.length() - 1);
   }

   if ((off == true) && (toggleTable_.find(setting) == toggleTable_.end()))
   {
      setting = setting.substr(2, setting.length() - 2);
   }

   if (toggleTable_.find(setting) != toggleTable_.end())
   {
      Setting<bool> * const set = toggleTable_[setting];

      if (toggle == true)
      {
         set->Set(!(set->Get()));
      }
      else
      {
         set->Set((off == false));
      }
   }
   else
   {
      Error(1, "No such setting: " + setting);
   }
}


Ui::Screen::MainWindow Settings::Window() const
{
   return defaultWindow_;
}

bool Settings::AutoScroll() const
{
   return Get(AutoScrollSetting);
}

bool Settings::StopOnQuit() const
{
   return Get(StopOnQuitSetting);
}

bool Settings::HightlightSearch() const
{
   return Get(HighlightSearchSetting);
}


void Settings::SetWindow(std::string const & arguments)
{
   std::string window(arguments);
   std::transform(window.begin(), window.end(), window.begin(), ::tolower);

   defaultWindow_ = Ui::Screen::GetWindowFromName(window);
}
