/*
   Vimpc
   Copyright (C) 2010 - 2011 Nathan Sweetman

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
#include "window/error.hpp"

#include <algorithm>
#include <pcrecpp.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>

using namespace Main;

// \TODO some settings i would like to add are commented out here
//       once they are implemented this can be removed

char const * const AutoScrollSetting       = "autoscroll";
//char const * const BrowseNumbersSetting    = "browsenumbers"; //disable song numbers in browse
char const * const ColourSetting           = "colour";
char const * const ExpandArtistsSetting    = "expand-artists";
//char const * const GraphicalVolumeSetting  = "graphicalvolume"; //Display volume graphically (colours) something like [||||||||  ][80%]
//char const * const HideTabBarSetting       = "hidetabbar"; //Hide the tab bar
char const * const HighlightSearchSetting  = "hlsearch";
char const * const IgnoreCaseSearchSetting = "ignorecase";
//char const * const PlaylistNumbersSetting  = "playlistnumbers"; //disable song numbers in playlist
char const * const SearchWrapSetting       = "searchwrap";
char const * const StopOnQuitSetting       = "stoponquit";
//char const * const TimeRemainingSetting    = "timeremaining"; // Show time remaining instead of played
char const * const WindowNumbersSetting    = "windownumbers";

bool skipConfigConnects_ (false);

Settings & Settings::Instance()
{
   static Settings settings_;
   return settings_;
}

Settings::Settings() :
   defaultWindow_(Ui::Screen::Playlist),
   settingsTable_(),
   toggleTable_  ()
{
   settingsTable_["window"]              = &Settings::SetWindow;

   toggleTable_[AutoScrollSetting]       = new Setting<bool>(true);
   toggleTable_[ColourSetting]           = new Setting<bool>(true);
   toggleTable_[ExpandArtistsSetting]    = new Setting<bool>(false);
   toggleTable_[HighlightSearchSetting]  = new Setting<bool>(true);
   toggleTable_[IgnoreCaseSearchSetting] = new Setting<bool>(false);
   toggleTable_[SearchWrapSetting]       = new Setting<bool>(true);
   toggleTable_[StopOnQuitSetting]       = new Setting<bool>(true);
   toggleTable_[WindowNumbersSetting]    = new Setting<bool>(false);
}

Settings::~Settings()
{
   for (SettingsToggleTable::iterator it = toggleTable_.begin(); it != toggleTable_.end(); ++it)
   {
      delete it->second;
   }
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
      Error(ErrorNumber::SettingNonexistant, "No such setting: " + setting);
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
      Error(ErrorNumber::SettingNonexistant, "No such setting: " + setting);
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

bool Settings::ColourEnabled() const
{
   return Get(ColourSetting);
}

bool Settings::ExpandArtists() const
{
   return Get(ExpandArtistsSetting);
}

bool Settings::HightlightSearch() const
{
   return Get(HighlightSearchSetting);
}

bool Settings::IgnoreCaseSearch() const
{
   return Get(IgnoreCaseSearchSetting);
}

bool Settings::SearchWrap() const
{
   return Get(SearchWrapSetting);
}

bool Settings::StopOnQuit() const
{
   return Get(StopOnQuitSetting);
}

bool Settings::WindowNumbers() const
{
   return Get(WindowNumbersSetting);
}


void Settings::SetWindow(std::string const & arguments)
{
   std::string window(arguments);
   std::transform(window.begin(), window.end(), window.begin(), ::tolower);

   defaultWindow_ = Ui::Screen::GetWindowFromName(window);
}

void Settings::SetSkipConfigConnects(bool val)
{
   skipConfigConnects_ = val;
}

bool Settings::SkipConfigConnects() const
{
   return skipConfigConnects_;
}
