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

   settings.hpp - handle configuration options via :set command
   */

#ifndef __MAIN__SETTINGS
#define __MAIN__SETTINGS

#include "screen.hpp"

#include <string>
#include <map>

// Create an enum entry, name and value for each settinf
// X(enum-entry, setting-name, default-value)
#define TOGGLE_SETTINGS \
   X(AutoScroll,       "autoscroll",     true)  /* Automatically scroll to playing song */ \
   X(BrowseNumbers,    "browsenumbers",  true)  /* Show numbers in the browse window */ \
   X(ColourEnabled,    "colour",         true)  /* Determine if we should use colours */ \
   X(ExpandArtists,    "expand-artists", false) /* Expand artists in the library window by default */ \
   X(HighlightSearch,  "hlsearch",       true)  /* Show search results in a different colour */ \
   X(IgnoreCaseSearch, "ignorecase",     false) /* Turn off case sensitivity on searching */ \
   X(IgnoreCaseSort,   "sortignorecase", true)  /* Turn off case sensitivity on sorting */\
   X(IgnoreTheSort,    "sortignorethe",  false) /* Ignore 'the' when sorting */ \
   X(IncrementalSearch,"incsearch",      false) /* Search for results whilst typing */ \
   X(Mouse,            "mouse",          true)  /* Handle mouse keys */ \
   X(Polling,          "polling",        false) /* Poll for status updates */ \
   X(PlaylistNumbers,  "playlistnumbers",true)  /* Show id next to each song in the playlist */ \
   X(Reconnect,        "reconnect",      true)  /* Reconnect to server when connection drops */ \
   X(ScrollOnAdd,      "scrollonadd",    true)  /* Auto scroll down after song added */ \
   X(ScrollOnDelete,   "scrollondelete", true)  /* Auto scroll down after song delete */ \
   X(SearchWrap,       "searchwrap",     true)  /* Determine whether to wrap searching */ \
   X(SingleQuit,       "singlequit",     false) /* Quit the entire application not just close a tab */ \
   X(SongNumbers,      "songnumbers",    true)  /* Show id numbers next to songs in any window */ \
   X(SmartCase,        "smartcase",      false) /* Case sensitivy enabled when upper case char is used */  \
   X(StopOnQuit,       "stoponquit",     false) /* Stop playing when we quit */ \
   X(TabBar,           "tabbar",         true)  /* Show the tab bar */ \
   X(TimeRemaining,    "timeremaining",  false) /* Show time left rather than time elapsed */ \
   X(WindowNumbers,    "windownumbers",  false) /* Window numbers next to each window in the tab list */

// X(enum-entry, setting-name, default-value, regex-filter)
#define STRING_SETTINGS \
   /* position to add songs */ \
   X(AddPosition,      "add", "end", "next|end") \
   /* Library format string */ \
   X(AlbumFormat,      "albumformat", "%B",  ".*") \
   /* Library format string */ \
   X(ArtistFormat,     "artistformat", "%A",  ".*") \
   /* Library format string */ \
   X(LibraryFormat,    "libraryformat", "$H[$H%l$H]$H {%t}|{%f}$E$R ", ".*") \
   /* Song format string */ \
   X(SongFormat,       "songformat", "{%a - %t}|{%f}$E$R $H[$H%l$H]$H", ".*") \
   /* Sort based on song format */ \
   X(Sort,             "sort", "format", "format|library") \
   /* Connection Timeout in seconds */ \
   X(Timeout,          "timeout", "15", "\\d+") \
   /* Startup window */ \
   X(Window,           "window",  "playlist", ".*")

//! \TODO convert parameter validation to using regular expression specified in the X macro

class Setting
{
public:
   // Use for add position
   static std::string AddEnd;
   static std::string AddNext;

#define X(a, b, c) a,
   typedef enum
   {
      TOGGLE_SETTINGS
      ToggleCount
   } ToggleSettings;
#undef X

#define X(a, b, c, d) a,
   typedef enum
   {
      StartString = ToggleCount,
      STRING_SETTINGS
      StringCount
   } StringSettings;
#undef X
};
                      
namespace Main
{
   //! Holds the value of an individual setting
   template <typename T>
   class SettingValue
   {
      public:
         SettingValue() { }
         SettingValue(T v) : value_(v) { }
         ~SettingValue() { }

         T Get() const { return value_; }
         void Set(T v) { value_ = v; }

      private:
         T value_;
   };

   //! Manages settings which are set via :set command
   class Settings
   {
      private:
         typedef std::map<std::string, SettingValue<bool> * >        BoolSettingsTable;
         typedef std::map<std::string, SettingValue<std::string> * > StringSettingsTable;

      public:
         static Settings & Instance();

      protected:
         Settings();
         ~Settings();

      public:
         //! List all the available settings (including no versions);
         std::vector<std::string> AvailableSettings() const;

         //! Calls the correct setter function based upon the given input
         void Set(std::string const & input);

         //! Get the value of a particular setting
         bool Get(Setting::ToggleSettings setting) const;
         std::string Get(Setting::StringSettings setting) const;

      public:
         //! Set/Get whether or not to connect if asked to in config
         void SetSkipConfigConnects(bool val);
         bool SkipConfigConnects() const;

      private:
         //! Used to handle settings that require very specific paramters
         void SetSpecificSetting(std::string setting, std::string arguments);

         //! Handles settings which are treated as an on/off setting
         void SetSingleSetting(std::string setting);

      public:
         //! Get the value for the given \p setting
         bool GetBool(std::string setting) const 
         { 
            BoolSettingsTable::const_iterator it = toggleTable_.find(setting);
            return ((it != toggleTable_.end()) && (it->second->Get()));
         }

         std::string GetString(std::string setting) const 
         { 
            StringSettingsTable::const_iterator it = stringTable_.find(setting);
            if (it != stringTable_.end())
            {
               return (it->second->Get());
            }
            return "";
         }

      private:
         bool AddPositionFilter(std::string arguments) const;
         bool SortFilter(std::string arguments) const;

      private:
         typedef std::map<int, std::string> SettingNameTable;
         SettingNameTable     settingName_;

         // Used to validate non boolean settings
         typedef std::map<std::string, std::string> SettingsFilterTable;
         SettingsFilterTable  filterTable_;

         // Holds yes/no, on/off style settings
         BoolSettingsTable    toggleTable_;

         // Settings that are represented by a string value
         StringSettingsTable  stringTable_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
