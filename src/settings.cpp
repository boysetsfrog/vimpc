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
#include "screen.hpp"
#include "window/debug.hpp"
#include "window/error.hpp"
#include "window/result.hpp"

#include <algorithm>
#include <pcrecpp.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>

using namespace Main;

bool skipConfigConnects_ (false);

std::string Setting::AddEnd  = "end";
std::string Setting::AddNext = "next";

std::string Setting::PlaylistsMpd   = "mpd";
std::string Setting::PlaylistsAll   = "all";
std::string Setting::PlaylistsFiles = "files";

Settings & Settings::Instance()
{
   static Settings settings_;
   return settings_;
}

Settings::Settings() :
   toggleTable_  (),
   stringTable_  ()
{
#define X(a, b, c) toggleTable_[b] = new SettingValue<bool>(Setting::a, c); \
                   settingName_[Setting::a] = b;
   TOGGLE_SETTINGS
#undef X

#define X(a, b, c, d) stringTable_[b] = new SettingValue<std::string>(Setting::a, c); \
                      settingName_[Setting::a] = b; \
                      filterTable_[b] = d;
   STRING_SETTINGS
#undef X

#define X(a, b) colourTable_[b] = a;
   COLOUR_SETTINGS
#undef X

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
   pcrecpp::RE const printCheck ("^.*\\?$");

   std::string       setting, arguments;
   std::stringstream settingStream(input);

   std::getline(settingStream, setting,   ' ');
   std::getline(settingStream, arguments, '\n');

   bool const print (printCheck.FullMatch(setting.c_str()));

   // If the setting is followed by '?' then
   // Show the current value of the setting rather than changing it
   if (print == true)
   {
      // Remove the '?' from the end of the setting
      setting = setting.substr(0, setting.length() - 1);

      // Print the value either "setting" or "nosetting"
      if (toggleTable_.find(setting) != toggleTable_.end())
      {
         SettingValue<bool> * const set = toggleTable_[setting];
         Result(std::string("  ") + ((set->Get()) ? "" : "no") + setting);
      }
      // Print the settings string value
      else if (stringTable_.find(setting) != stringTable_.end())
      {
         SettingValue<std::string> * const set = stringTable_[setting];
         Result(std::string("  ") + setting + std::string("=") + set->Get());
      }
      else
      {
         ErrorString(ErrorNumber::SettingNonexistant, setting);
      }
   }
   else if (arguments == "")
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
   SettingNameTable::const_iterator const it = settingName_.find(setting);

   if (it != settingName_.end())
   {
      return GetBool(it->second);
   }

   ASSERT(false);
   return false;
}

std::string Settings::Get(::Setting::StringSettings setting) const
{
   SettingNameTable::const_iterator const it = settingName_.find(setting);

   if (it != settingName_.end())
   {
      return GetString(it->second);
   }

   ASSERT(false);
   return "";
}


void Settings::RegisterCallback(Setting::ToggleSettings setting, BoolCallback callback) const
{
   tCallbackTable_[setting].push_back(callback);
}

void Settings::RegisterCallback(Setting::StringSettings setting, StringCallback callback) const
{
   sCallbackTable_[setting].push_back(callback);
}


void Settings::SetSkipConfigConnects(bool val)
{
   skipConfigConnects_ = val;
}

bool Settings::SkipConfigConnects() const
{
   return skipConfigConnects_;
}

void Settings::SetColour(std::string property, std::string colour)
{
   if (colourTable_.find(colour) != colourTable_.end())
   {
      if (property == "song") {
         colours.Song = colourTable_[colour];
      }
      else if (property == "id") {
         colours.SongId = colourTable_[colour];
      }
      else if (property == "dir") {
         colours.Directory = colourTable_[colour];
      }
      else if (property == "current") {
         colours.CurrentSong = colourTable_[colour];
      }
      else if (property == "match") {
         colours.SongMatch = colourTable_[colour];
      }
      else if (property == "partial") {
         colours.PartialAdd = colourTable_[colour];
      }
      else if (property == "full") {
         colours.FullAdd = colourTable_[colour];
      }
      else if (property == "pager") {
         colours.PagerStatus = colourTable_[colour];
      }
      else if (property == "error") {
         colours.Error = colourTable_[colour];
      }
      else if (property == "status") {
         colours.StatusLine = colourTable_[colour];
      }
      else if (property == "tab") {
         colours.TabWindow = colourTable_[colour];
      }
      else {
         ErrorString(ErrorNumber::UnknownOption, property);
      }
   }
   else
   {
         ErrorString(ErrorNumber::UnknownOption, colour);
   }
}

void Settings::SetSpecificSetting(std::string setting, std::string arguments)
{
   if (stringTable_.find(setting) != stringTable_.end())
   {
      bool ValidSetting = true;

      // Validate the arguments using regex defined in settings table
      if (filterTable_.find(setting) != filterTable_.end())
      {
         pcrecpp::RE const filterCheck(filterTable_[setting]);
         ValidSetting = (filterCheck.FullMatch(arguments));
      }

      if (ValidSetting == true)
      {
         SettingValue<std::string> * const set = stringTable_[setting];

         if (set != NULL)
         {
            Debug("Setting " + setting + " to " + arguments); 
            set->Set(arguments);

            // Call any registered callbacks for this setting
            std::vector<StringCallback> Callbacks = 
               sCallbackTable_[static_cast<Setting::StringSettings>(set->Id())];

            for (std::vector<StringCallback>::iterator it = Callbacks.begin(); it != Callbacks.end(); ++it)
            {
               StringCallback functor = (*it);
               (*functor)(arguments);
            }
         }
         else
         {
            ASSERT(false);
         }
      }
      else
      {
         ErrorString(ErrorNumber::UnknownOption, arguments);
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

   // If the setting is followed by '!' toggle it's current value
   bool const toggle(toggleCheck.FullMatch(setting.c_str()));

   // If the setting is prefixed "no" turn the setting off
   bool const off   (offCheck.FullMatch(setting.c_str()));

   // Remove any extra characters from the settings name
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

      bool newValue = (off == false);

      if (toggle == true)
      {
         newValue = !(set->Get());
      }

      if (newValue != set->Get())
      {
         set->Set(newValue);

         // Call any registered callbacks for this setting
         std::vector<BoolCallback> Callbacks = 
            tCallbackTable_[static_cast<Setting::ToggleSettings>(set->Id())];

         for (std::vector<BoolCallback>::iterator it = Callbacks.begin(); it != Callbacks.end(); ++it)
         {
            BoolCallback functor = (*it);
            (*functor)(newValue);
         }
      }
   }
   else
   {
      ErrorString(ErrorNumber::SettingNonexistant, setting);
   }
}

/* vim: set sw=3 ts=3: */
