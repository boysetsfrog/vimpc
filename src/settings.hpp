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

   settings.hpp - handle configuration options via :set command
   */

#ifndef __MAIN__SETTINGS
#define __MAIN__SETTINGS

#include "screen.hpp"

#include <string>
#include <map>

namespace Main
{
   //! Holds the value of an individual setting
   template <typename T>
   class Setting
   {
      public:
         Setting() { }
         Setting(T v) : value_(v) { }
         ~Setting() { }

         T Get() const { return value_; }
         void Set(T v) { value_ = v; }

      private:
         T value_;
   };

   //! Manages settings which are set via :set command
   class Settings
   {
      private:
         typedef std::map<std::string, Setting<bool> * > SettingsToggleTable;

      public:
         static Settings & Instance();

      protected:
         Settings();
         ~Settings();

      public:
         //! Calls the correct setter function based upon the given input
         void Set(std::string const & input);

      public: //Specific Settings
         //! Gets the default startup window
         std::string Window() const;

      public: //Toggle Settings
         //! Determine whether to autmatically scroll to playing song
         bool AutoScroll() const;

         //! Show numbers in the browse window
         bool BrowseNumbers() const;

         //! Determine if we should use colours
         bool ColourEnabled() const;

         //! Should we expand artists in the library window by default
         bool ExpandArtists() const;

         //! Hide the tab bar from the display
         bool HideTabBar() const;

         //! Determine whether to show search results in a different colour
         bool HightlightSearch() const;

         //! Turn off case sensitivity on searching
         bool IgnoreCaseSearch() const;

         //! Turn off case sensitivity on sorting
         bool IgnoreCaseSort() const;

         //! Ignore 'the' when sorting
         bool IgnoreTheSort() const;

         //! Determine whether to wrap searching
         bool SearchWrap() const;

         //! Show id next to each song in the playlist
         bool PlaylistNumbers() const;

         //! Quit will quit the entire application not just close a tab
         bool SingleQuit() const;

         //! If used with ignore case, case sensitivy search is re-enabled when an upper case
         //! character is used in the search string
         bool SmartCase() const;

         //! Show id numbers next to songs in any window
         bool SongNumbers() const;

         //! Determines whether we should stop playing when we quit
         bool StopOnQuit() const;

         //! Show time left rather than time elapsed
         bool TimeRemaining() const;

         //! Show window numbers next to each window in the tab list
         bool WindowNumbers() const;

         //! Set/Get whether or not to connect if asked to in config
         void SetSkipConfigConnects(bool val);
         bool SkipConfigConnects() const;


      private:
         //! Used to handle settings that require very specific paramters
         void SetSpecificSetting(std::string setting, std::string arguments);

         //! Handles settings which are treated as an on/off setting
         void SetSingleSetting(std::string setting);

      private:
         //! Get the value for the given \p setting
         bool Get(std::string setting) const 
         { 
            SettingsToggleTable::const_iterator it = toggleTable_.find(setting);
            return ((it != toggleTable_.end()) && (it->second->Get()));
         }

      private:
         //! Sets the startup window
         void SetWindow(std::string const & arguments);

      private:
         std::string defaultWindow_;

         typedef void (Main::Settings::*ptrToMember)(std::string const &);
         typedef std::map<std::string, ptrToMember> SettingsFunctionTable;
         SettingsFunctionTable settingsTable_;

         SettingsToggleTable   toggleTable_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
