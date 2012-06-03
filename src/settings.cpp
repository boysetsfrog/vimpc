/*
   Vimpc
   Copyright (C) 2010 - 2012 Nathan Sweetman

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
#include "window/debug.hpp"
#include "window/error.hpp"

#include <algorithm>
#include <pcrecpp.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>

using namespace Main;

bool skipConfigConnects_ (false);

std::string Setting::AddEnd  = "end";
std::string Setting::AddNext = "next";

Settings & Settings::Instance()
{
   static Settings settings_;
   return settings_;
}

Settings::Settings() :
   toggleTable_  (),
   stringTable_  ()
{
#define X(a, b, c) toggleTable_[b] = new SettingValue<bool>(c); \
                   settingName_[Setting::a] = b;
   TOGGLE_SETTINGS
#undef X

#define X(a, b, c) stringTable_[b] = new SettingValue<std::string>(c); \
                   settingName_[Setting::a] = b;
   STRING_SETTINGS
#undef X

   filterTable_[settingName_[::Setting::AddPosition]] = &Settings::AddPositionFilter;
   filterTable_[settingName_[::Setting::Sort]]        = &Settings::SortFilter;
}

Settings::~Settings()
{
   for (BoolSettingsTable::iterator it = toggleTable_.begin(); it != toggleTable_.end(); ++it)
   {
      delete it->second;
   }

   for (StringSettingsTable::iterator it = stringTable_.begin(); it != stringTable_.end(); ++it)
   {
      delete it->second;
   }
}


std::vector<std::string> Settings::AvailableSettings() const
{
   std::vector<std::string> AllSettings;

   for (BoolSettingsTable::const_iterator it = toggleTable_.begin(); it != toggleTable_.end(); ++it)
   {
      AllSettings.push_back(it->first);
      AllSettings.push_back("no" + it->first);
   }

   for (StringSettingsTable::const_iterator it = stringTable_.begin(); it != stringTable_.end(); ++it)
   {
      AllSettings.push_back(it->first);
   }

   std::sort(AllSettings.begin(), AllSettings.end());

   return AllSettings;
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

bool Settings::Get(::Setting::ToggleSettings setting) const
{
   SettingNameTable::const_iterator it = settingName_.find(setting);

   if (it != settingName_.end())
   {
      return GetBool(it->second);
   }

   ASSERT(false);
   return false;
}

std::string Settings::Get(::Setting::StringSettings setting) const
{
   SettingNameTable::const_iterator it = settingName_.find(setting);

   if (it != settingName_.end())
   {
      return GetString(it->second);
   }

   ASSERT(false);
   return "";
}

void Settings::SetSpecificSetting(std::string setting, std::string arguments)
{
   if (stringTable_.find(setting) != stringTable_.end())
   {
      bool ValidSetting = true;

      // Validate the arguments
      if (filterTable_.find(setting) != filterTable_.end())
      {
         Debug("Setting checked for setting for " + setting); 

         SettingsFilterFunction const function = filterTable_[setting];
         ValidSetting = (*this.*function)(arguments);
      }

      if (ValidSetting == true)
      {
         Debug("Valid setting " + setting + " - " + arguments); 
         SettingValue<std::string> * const set = stringTable_[setting];
         set->Set(arguments);
      }
      else
      {
         ErrorString(ErrorNumber::InvalidParameter, arguments);
      }
   }
   else
   {
      ErrorString(ErrorNumber::SettingNonexistant, setting);
   }
}

void Settings::SetSingleSetting(std::string setting)
{
   pcrecpp::RE const toggleCheck("^.*!$");
   pcrecpp::RE const offCheck   ("^no.*");

   bool const toggle(toggleCheck.FullMatch(setting.c_str()));
   bool const off   (offCheck.FullMatch(setting.c_str()));

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
      SettingValue<bool> * const set = toggleTable_[setting];

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
      ErrorString(ErrorNumber::SettingNonexistant, setting);
   }
}

void Settings::SetSkipConfigConnects(bool val)
{
   skipConfigConnects_ = val;
}

bool Settings::SkipConfigConnects() const
{
   return skipConfigConnects_;
}

bool Settings::AddPositionFilter(std::string arguments) const
{
   return ((arguments == "end") || (arguments == "next"));
}

bool Settings::SortFilter(std::string arguments) const
{
   return ((arguments == "format") || (arguments == "library"));
}

/* vim: set sw=3 ts=3: */
