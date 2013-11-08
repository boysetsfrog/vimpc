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
#include <iostream>
#include <sstream>
#include <stdlib.h>

#include "regex.hpp"

using namespace Main;

bool skipConfigConnects_ (false);

std::string Setting::Default = "SettingDefaultValueKey";

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
                   toggleVector_.push_back(toggleTable_[b]); \
                   settingName_[Setting::a] = b;
   TOGGLE_SETTINGS
#undef X

#define X(a, b, c, d) stringTable_[b] = new SettingValue<std::string>(Setting::a, c); \
                      stringVector_.push_back(stringTable_[b]); \
                      settingName_[Setting::a] = b; \
                      filterTable_[b] = d;
   STRING_SETTINGS
#undef X

#define X(a, b) colourTable_[b] = a;
   COLOUR_SETTINGS
#undef X

   stringTable_[Setting::Default] = new SettingValue<std::string>(-1, "");
   toggleTable_[Setting::Default] = new SettingValue<bool>(-1, false);

   enabled_ = true;
}

Settings::~Settings()
{
   mutex_.lock();

   for (auto entry : toggleTable_)
   {
      delete entry.second;
   }

   for (auto entry : stringTable_)
   {
      delete entry.second;
   }

   mutex_.unlock();
}


std::vector<std::string> Settings::AvailableSettings() const
{
   mutex_.lock();

   std::vector<std::string> AllSettings;

   for (auto entry : toggleTable_)
   {
      if (entry.first != Setting::Default)
      {
         AllSettings.push_back(entry.first);
         AllSettings.push_back("no" + entry.first);
      }
   }

   for (auto entry : stringTable_)
   {
      if (entry.first != Setting::Default)
      {
         AllSettings.push_back(entry.first);
      }
   }

   mutex_.unlock();

   std::sort(AllSettings.begin(), AllSettings.end());

   return AllSettings;
}


void Settings::Set(std::string const & input)
{
   Regex::RE const printCheck ("^.*\\?$");

   std::string       setting, arguments;
   std::stringstream settingStream(input);

   std::getline(settingStream, setting,   ' ');
   std::getline(settingStream, arguments, '\n');

   bool const print (printCheck.Matches(setting.c_str()));

   // If the setting is followed by '?' then
   // Show the current value of the setting rather than changing it
   if (print == true)
   {
      // Remove the '?' from the end of the setting
      setting = setting.substr(0, setting.length() - 1);

      mutex_.lock();

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
         mutex_.unlock();
         ErrorString(ErrorNumber::SettingNonexistant, setting);
         return;
      }

      mutex_.unlock();
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
   bool Result = false;

   mutex_.lock();

   if ((setting >= 0) && (setting < toggleVector_.size()))
   {
      Result = toggleVector_.at(setting)->Get();
   }
   else
   {
      ASSERT(false);
   }

   mutex_.unlock();

   return Result;
}

std::string Settings::Get(::Setting::StringSettings setting) const
{
   uint32_t const strSetting = setting - (Setting::StartString + 1);

   std::string Result = "";

   mutex_.lock();

   if (strSetting < stringVector_.size())
   {
      Result = stringVector_.at(strSetting)->Get();
   }
   else
   {
      ASSERT(false);
   }

   mutex_.unlock();

   return Result;
}

void Settings::Set(::Setting::ToggleSettings setting, bool value)
{
   mutex_.lock();

   auto const it = settingName_.find(setting);

   if (it != settingName_.end())
   {
      SetValue(it->second, value);
   }
   else
   {
      ASSERT(false);
   }

   mutex_.unlock();
}

void Settings::Set(::Setting::StringSettings setting, std::string value)
{
   mutex_.lock();

   auto const it = settingName_.find(setting);

   if (it != settingName_.end())
   {
      SetValue(it->second, value);
   }
   else
   {
      ASSERT(false);
   }

   mutex_.unlock();
}

std::string Settings::Name(::Setting::ToggleSettings setting) const
{
   std::string Result = "";

   mutex_.lock();

   auto const it = settingName_.find(setting);

   if (it != settingName_.end())
   {
      Result = it->second;
   }

   mutex_.unlock();

   return Result;
}

std::string Settings::Name(::Setting::StringSettings setting) const
{
   std::string Result = "";

   mutex_.lock();

   auto const it = settingName_.find(setting);

   if (it != settingName_.end())
   {
      Result = it->second;
   }

   mutex_.unlock();

   return Result;
}


void Settings::RegisterCallback(Setting::ToggleSettings setting, FUNCTION<void (bool)> callback)
{
   mutex_.lock();
   tCallbackTable_[setting].push_back(callback);
   mutex_.unlock();
}

void Settings::RegisterCallback(Setting::StringSettings setting, FUNCTION<void (std::string)> callback)
{
   mutex_.lock();
   sCallbackTable_[setting].push_back(callback);
   mutex_.unlock();
}

void Settings::EnableCallbacks()
{
   mutex_.lock();
   enabled_ = true;
   mutex_.unlock();
}

void Settings::DisableCallbacks()
{
   mutex_.lock();
   enabled_ = false;
   mutex_.unlock();
}


void Settings::SetSkipConfigConnects(bool val)
{
   mutex_.lock();
   skipConfigConnects_ = val;
   mutex_.unlock();
}

bool Settings::SkipConfigConnects() const
{
   mutex_.lock();
   bool const skipValue = skipConfigConnects_;
   mutex_.unlock();

   return skipValue;
}

void Settings::SetColour(std::string property, std::string colour)
{
   mutex_.lock();

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
      else if (property == "progress") {
         colours.ProgressWindow = colourTable_[colour];
      }
      else
      {
         mutex_.unlock();
         ErrorString(ErrorNumber::UnknownOption, property);
         return;
      }
   }
   else
   {
      mutex_.unlock();
      ErrorString(ErrorNumber::UnknownOption, colour);
      return;
   }

   mutex_.unlock();
}

void Settings::SetSpecificSetting(std::string setting, std::string arguments)
{
   mutex_.lock();

   if (stringTable_.find(setting) != stringTable_.end())
   {
      bool ValidSetting = true;

      // Validate the arguments using regex defined in settings table
      if (filterTable_.find(setting) != filterTable_.end())
      {
         Regex::RE const filterCheck(filterTable_[setting]);
         ValidSetting = (filterCheck.CompleteMatch(arguments));
      }

      if (ValidSetting == true)
      {
         SettingValue<std::string> * const set = stringTable_[setting];

         if (set != NULL)
         {
            Debug("Setting %s to %s", setting.c_str(), arguments.c_str());
            set->Set(arguments);

            if (enabled_ == true)
            {
               // Call any registered callbacks for this setting
               auto const Callbacks = sCallbackTable_[static_cast<Setting::StringSettings>(set->Id())];

               for (auto func : Callbacks)
               {
                  (func)(arguments);
               }
            }
         }
         else
         {
            ASSERT(false);
         }
      }
      else
      {
         mutex_.unlock();
         ErrorString(ErrorNumber::UnknownOption, arguments);
         return;
      }
   }
   else
   {
      mutex_.unlock();
      ErrorString(ErrorNumber::SettingNonexistant, setting);
      return;
   }

   mutex_.unlock();
}

void Settings::SetSingleSetting(std::string setting)
{
   Regex::RE const toggleCheck("^.*!$");
   Regex::RE const offCheck   ("^no.*");

   // If the setting is followed by '!' toggle it's current value
   bool const toggle(toggleCheck.Matches(setting.c_str()));

   // If the setting is prefixed "no" turn the setting off
   bool const off   (offCheck.Matches(setting.c_str()));

   // Remove any extra characters from the settings name
   if (toggle == true)
   {
      setting = setting.substr(0, setting.length() - 1);
   }

   if ((off == true) && (toggleTable_.find(setting) == toggleTable_.end()))
   {
      setting = setting.substr(2, setting.length() - 2);
   }

   mutex_.lock();

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

         if (enabled_ == true)
         {
            // Call any registered callbacks for this setting
            auto const Callbacks = tCallbackTable_[static_cast<Setting::ToggleSettings>(set->Id())];

            for (auto func : Callbacks)
            {
               (func)(newValue);
            }
         }
      }
   }
   else
   {
      mutex_.unlock();
      ErrorString(ErrorNumber::SettingNonexistant, setting);
      return;
   }

   mutex_.unlock();
}

/* vim: set sw=3 ts=3: */
