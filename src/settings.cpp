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

char const * const sAutoScroll       = "autoscroll";
char const * const sBrowseNumbers    = "browsenumbers"; //disable song numbers in browse
char const * const sColour           = "colour";
char const * const sExpandArtists    = "expand-artists";
char const * const sGraphicalVolume  = "graphicalvolume"; // \todo Display volume graphically (colours) something like [||||||||  ][80%]
char const * const sHideTabBar       = "hidetabbar"; // \todo this is going to be quite hard, we do dodgy hacks with MaxRows everywhere!
char const * const sHighlightSearch  = "hlsearch";
char const * const sIgnoreCaseSearch = "ignorecase";
char const * const sIgnoreCaseSort   = "sortignorecase";
char const * const sIgnoreTheSort    = "sortignorethe";
char const * const sPlaylistNumbers  = "playlistnumbers"; //disable song numbers in playlist
char const * const sPolling          = "polling"; 
char const * const sSearchWrap       = "searchwrap";
char const * const sSingleQuit       = "singlequit";
char const * const sSongNumbers      = "songnumbers";
char const * const sSmartCase        = "smartcase";
char const * const sStopOnQuit       = "stoponquit";
char const * const sTimeRemaining    = "timeremaining";
char const * const sWindowNumbers    = "windownumbers";

bool skipConfigConnects_ (false);

Settings & Settings::Instance()
{
   static Settings settings_;
   return settings_;
}

Settings::Settings() :
   defaultWindow_("playlist"),
   songFormat_   ("{%a - %t}|{%f}$R$H[$H%l$H]$H"),
   libFormat_    ("$H[$H%l$H]$H {%t}|{%f}"),
   settingsTable_(),
   toggleTable_  ()
{
   settingsTable_["window"]        = &Settings::SetWindow;
   settingsTable_["songformat"]    = &Settings::SetSongFormat;
   settingsTable_["libraryformat"] = &Settings::SetLibFormat;

   toggleTable_[sAutoScroll]       = new Setting<bool>(true);
   toggleTable_[sBrowseNumbers]    = new Setting<bool>(true);
   toggleTable_[sColour]           = new Setting<bool>(true);
   toggleTable_[sExpandArtists]    = new Setting<bool>(false);
   toggleTable_[sHideTabBar]       = new Setting<bool>(false);
   toggleTable_[sHighlightSearch]  = new Setting<bool>(true);
   toggleTable_[sIgnoreCaseSearch] = new Setting<bool>(false);
   toggleTable_[sIgnoreCaseSort]   = new Setting<bool>(true);
   toggleTable_[sIgnoreTheSort]    = new Setting<bool>(false);
   toggleTable_[sPolling]          = new Setting<bool>(true);
   toggleTable_[sPlaylistNumbers]  = new Setting<bool>(true);
   toggleTable_[sSearchWrap]       = new Setting<bool>(true);
   toggleTable_[sSingleQuit]       = new Setting<bool>(false);
   toggleTable_[sSongNumbers]      = new Setting<bool>(true);
   toggleTable_[sSmartCase]        = new Setting<bool>(false);
   toggleTable_[sStopOnQuit]       = new Setting<bool>(false);
   toggleTable_[sTimeRemaining]    = new Setting<bool>(false);
   toggleTable_[sWindowNumbers]    = new Setting<bool>(false);
}

Settings::~Settings()
{
   for (SettingsTable::iterator it = toggleTable_.begin(); it != toggleTable_.end(); ++it)
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


std::string Settings::Window() const
{
   return defaultWindow_;
}

std::string Settings::SongFormat() const
{
   return songFormat_;
}

std::string Settings::LibraryFormat() const
{
   return libFormat_;
}


bool Settings::AutoScroll() const
{
   return Get(sAutoScroll);
}

bool Settings::BrowseNumbers() const
{
   return Get(sBrowseNumbers);
}

bool Settings::ColourEnabled() const
{
   return Get(sColour);
}

bool Settings::ExpandArtists() const
{
   return Get(sExpandArtists);
}

bool Settings::HideTabBar() const
{
   return Get(sHideTabBar);
}

bool Settings::HightlightSearch() const
{
   return Get(sHighlightSearch);
}

bool Settings::IgnoreCaseSearch() const
{
   return Get(sIgnoreCaseSearch);
}

bool Settings::IgnoreCaseSort() const
{
   return Get(sIgnoreCaseSort);
}

bool Settings::IgnoreTheSort() const
{
   return Get(sIgnoreTheSort);
}

bool Settings::PlaylistNumbers() const
{
   return Get(sPlaylistNumbers);
}

bool Settings::SearchWrap() const
{
   return Get(sSearchWrap);
}

bool Settings::Polling() const
{
   return Get(sPolling);
}

bool Settings::SingleQuit() const
{
   return Get(sSingleQuit);
}

bool Settings::SmartCase() const
{
   return Get(sSmartCase);
}

bool Settings::SongNumbers() const
{
   return Get(sSongNumbers);
}

bool Settings::StopOnQuit() const
{
   return Get(sStopOnQuit);
}

bool Settings::TimeRemaining() const
{
   return Get(sTimeRemaining);
}

bool Settings::WindowNumbers() const
{
   return Get(sWindowNumbers);
}


void Settings::SetWindow(std::string const & arguments)
{
   std::string window(arguments);
   std::transform(window.begin(), window.end(), window.begin(), ::tolower);

   defaultWindow_ = window;
}

void Settings::SetSongFormat(std::string const & arguments)
{
   songFormat_ = arguments;
}

void Settings::SetLibFormat(std::string const & arguments)
{
   libFormat_ = arguments;
}

void Settings::SetSkipConfigConnects(bool val)
{
   skipConfigConnects_ = val;
}

bool Settings::SkipConfigConnects() const
{
   return skipConfigConnects_;
}
/* vim: set sw=3 ts=3: */
