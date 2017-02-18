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

#include "compiler.hpp"

#include "colours.hpp"
#include "test.hpp"

#include <string>
#include <map>
#include <vector>

// Create an enum entry, name and value for each setting
// X(enum-entry, setting-name, default-value)
#define TOGGLE_SETTINGS \
   X(AutoScroll,       "autoscroll",      true)  /* Automatically scroll to playing song */ \
   X(AutoUpdate,       "autoupdate",      true)  /* Automatically update after file edits */ \
   X(AutoLyrics,       "autolyrics",      false) /* Automatically get lyrics for the playing song */ \
   X(AutoScrollLyrics, "autoscrolllyrics",false) /* Automatically scroll lyrics for the playing song */ \
   X(AlbumArtist,      "albumartist",     true)  /* Use the album artist tag if there is one */ \
   X(BrowseNumbers,    "browsenumbers",   false) /* Show numbers in the browse window */ \
   X(ColourEnabled,    "colour",          true)  /* Determine if we should use colours */ \
   X(ExpandArtists,    "expand-artists",  false) /* Expand artists in the library window by default */ \
   X(HighlightSearch,  "hlsearch",        true)  /* Show search results in a different colour */ \
   X(IgnoreTheGroup,   "groupignorethe",  false) /* Ignore 'the' when grouping the same artist into library */ \
   X(IgnoreCaseSearch, "ignorecase",      false) /* Turn off case sensitivity on searching */ \
   X(IgnoreCaseSort,   "sortignorecase",  true)  /* Turn off case sensitivity on sorting */\
   X(IgnoreTheSort,    "sortignorethe",   false) /* Ignore 'the' when sorting */ \
   X(IncrementalSearch,"incsearch",       false) /* Search for results whilst typing */ \
   X(ListAllMeta,      "listallmeta",     true)  /* Get all meta data */ \
   X(Mouse,            "mouse",           true)  /* Handle mouse keys */ \
   X(Polling,          "polling",         false) /* Poll for status updates */ \
   X(PlaylistNumbers,  "playlistnumbers", true)  /* Show id next to each song in the playlist */ \
   X(PlayOnAdd,        "playonadd",       false) /* If mpd is stopped play after first add */ \
   X(ProgressBar,      "progressbar",     true)  /* Show the progress bar */ \
   X(Reconnect,        "reconnect",       true)  /* Reconnect to server when connection drops */ \
   X(ScrollOnAdd,      "scrollonadd",     true)  /* Auto scroll down after song added */ \
   X(ScrollOnDelete,   "scrollondelete",  true)  /* Auto scroll down after song delete */ \
   X(ScrollStatus,     "scrollstatus",    true)  /* Scroll long status lines */ \
   X(SearchWrap,       "searchwrap",      true)  /* Determine whether to wrap searching */ \
   X(SeekBar,          "seekbar",         true)  /* Mouse click on progress bar causes a seek */ \
   X(ShowPath,         "showpath",        true)  /* Show current path in directory window */ \
   X(ShowPercent,      "showpercent",     true)  /* Show percentage on the progress bar */ \
   X(ShowLists,        "showlists",       true)  /* Show playlists in directory window */ \
   X(SingleQuit,       "singlequit",      false) /* Quit the entire application not just close a tab */ \
   X(SongNumbers,      "songnumbers",     true)  /* Show id numbers next to songs in any window */ \
   X(SmartCase,        "smartcase",       false) /* Case sensitivy enabled when upper case char is used */  \
   X(StopOnQuit,       "stoponquit",      false) /* Stop playing when we quit */ \
   X(TabBar,           "tabbar",          true)  /* Show the tab bar */ \
   X(TimeRemaining,    "timeremaining",   false) /* Show time left rather than time elapsed */ \
   X(WindowNumbers,    "windownumbers",   false) /* Window numbers next to each window in the tab list */

// X(enum-entry, setting-name, default-value, regex-filter)
#define STRING_SETTINGS \
   /* position to add songs */ \
   X(AddPosition,      "add", "end", "next|end") \
   /* Library format string */ \
   X(AlbumFormat,      "albumformat", "%B",  ".*") \
   /* Library format string */ \
   X(ArtistFormat,     "artistformat", "%A",  ".*") \
   /* Library format string */ \
   X(LibraryFormat,    "libraryformat", "$I%n \\| $D$H[$H%l$H]$H {%t}|{%f}$E$R ", ".*") \
   /* Library format string */ \
   X(LocalMusicDir,    "local-music-dir", "", ".*") \
   /* Lyrics strip regex */ \
   X(LyricsStrip,      "lyricsstrip", "(\\s*(R|r)emaster\\w*)|(\\s+-.*)", ".*") \
   /* Lists to show in the lists window */ \
   X(Playlists,        "playlists", "mpd", "all|mpd|files") \
   /* Song format string */ \
   X(SongFormat,       "songformat", "{{%a - }%t}|{%f}$E$R $H[$H%l$H]$H", ".*") \
   /* Song format fill character */ \
   X(SongFillChar,       "songfillchar", " ", ".") \
   /* Sort based on song format */ \
   X(Sort,             "sort", "format", "format|library") \
   /* Connection Timeout in seconds */ \
   X(Timeout,          "timeout", "30", "\\d+") \
   /* Startup window */ \
   X(Window,           "window",  "help", ".*") \
   /* Startup windows */ \
   X(Windows,          "windows", "help,lists,library,browse,playlist", ".*")

#define COLOUR_SETTINGS \
   X(colours.DEFAULT_ON_BLACK, "blackbg" ) \
   X(colours.DEFAULT_ON_WHITE, "whitebg" ) \
   X(colours.DEFAULT_ON_RED, "redbg" ) \
   X(colours.DEFAULT_ON_GREEN, "greenbg" ) \
   X(colours.DEFAULT_ON_YELLOW, "yellowbg" ) \
   X(colours.DEFAULT_ON_BLUE, "bluebg" ) \
   X(colours.DEFAULT_ON_MAGENTA, "magentabg" ) \
   X(colours.DEFAULT_ON_CYAN, "cyanbg" ) \
   X(colours.BLACK_ON_DEFAULT, "blackfg" ) \
   X(colours.WHITE_ON_DEFAULT, "whitefg" ) \
   X(colours.RED_ON_DEFAULT, "redfg" ) \
   X(colours.GREEN_ON_DEFAULT, "greenfg" ) \
   X(colours.YELLOW_ON_DEFAULT, "yellowfg" ) \
   X(colours.BLUE_ON_DEFAULT, "bluefg" ) \
   X(colours.MAGENTA_ON_DEFAULT, "magentafg" ) \
   X(colours.CYAN_ON_DEFAULT, "cyanfg" ) \
   X(colours.DEFAULT, "default" )

class Setting
{
public:
   // Provide access to default values through settings table
   static std::string Default;

   // Use for add position comparisons
   static std::string AddEnd;
   static std::string AddNext;

   // Use for playlists comparisons
   static std::string PlaylistsMpd;
   static std::string PlaylistsAll;
   static std::string PlaylistsFiles;

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
         SettingValue(int32_t id) : id_(id) { }
         SettingValue(int32_t id, T v) : id_(id), value_(v) { }
         ~SettingValue() { }

         int32_t Id() const { return id_; }
         T Get() const { return value_; }
         void Set(T v) { value_ = v; }

      private:
         int32_t const id_;
         T             value_;
   };

   //! Manages settings which are set via :set command
   class Settings
   {
      public:
         Colours colours;

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

         //! Handles settings which are treated as an on/off setting
         void SetSingleSetting(std::string setting);

         //! Get the value of a particular setting
         bool Get(Setting::ToggleSettings setting) const;
         std::string Get(Setting::StringSettings setting) const;

         //! Name of a particular setting
         std::string Name(Setting::ToggleSettings setting) const;
         std::string Name(Setting::StringSettings setting) const;

         //! Register a callback to be called when a setting is changed
         void RegisterCallback(Setting::ToggleSettings setting, FUNCTION<void (bool)> callback);
         void RegisterCallback(Setting::StringSettings setting, FUNCTION<void (std::string)> callback);

         //! Turn the callbacks on and off
         void EnableCallbacks();
         void DisableCallbacks();

      public:
         //! Set/Get whether or not to connect if asked to in config
         void SetSkipConfigConnects(bool val);
         bool SkipConfigConnects() const;

         void SetColour(std::string property, std::string colour);

         //! Get the value for the given \p setting
         template <typename T>
         inline T Get(std::string setting) const;

         //! Set the value of a particular setting
         void Set(Setting::ToggleSettings setting, bool value);
         void Set(Setting::StringSettings setting, std::string value);

      protected:
         //! Set the value for the given \p setting
         inline void SetValue(std::string setting, bool value)
         {
            SetValue(setting, value, toggleTable_);
         }

         inline void SetValue(std::string setting, std::string value)
         {
            SetValue(setting, value, stringTable_);
         }

      private:
         template <class T>
         auto GetValue(std::string setting, T table) const -> decltype (table.at(Setting::Default)->Get())
         {
            mutex_.lock();
            auto const it = table.find(setting);
            auto const Result = (it != table.end()) ? (it->second->Get()) : table.at(Setting::Default)->Get();
            mutex_.unlock();
            return Result;
         }

         template <class T, class U>
         void SetValue(std::string setting, T value, U const & table)
         {
            mutex_.lock();
            auto const it = table.find(setting);
            if (it != table.end()) { (it->second->Set(value)); }
            mutex_.unlock();
         }

         //! Used to handle settings that require very specific paramters
         void SetSpecificSetting(std::string setting, std::string arguments);

      private:
         typedef std::map<int, std::string> SettingNameTable;
         SettingNameTable     settingName_;

         // Holds yes/no, on/off style settings
         typedef std::map<std::string, SettingValue<bool> * > BoolSettingsTable;
         BoolSettingsTable    toggleTable_;

         typedef std::vector<SettingValue<bool> * > BoolVector;
         BoolVector           toggleVector_;

         // Callbacks for on/off settings
         typedef std::map<Setting::ToggleSettings, std::vector<FUNCTION<void (bool)> > > BoolCallbackTable;
         BoolCallbackTable    tCallbackTable_;

         // Used to validate string settings against a regex pattern
         typedef std::map<std::string, std::string> SettingsFilterTable;
         SettingsFilterTable  filterTable_;

         // Settings that are represented by a string value
         typedef std::map<std::string, SettingValue<std::string> * > StringSettingsTable;
         StringSettingsTable  stringTable_;

         typedef std::vector<SettingValue<std::string> * > StringVector;
         StringVector         stringVector_;

         // Callbacks for string style settings
         typedef std::map<Setting::StringSettings, std::vector<FUNCTION<void (std::string)> > > StringCallbackTable;
         StringCallbackTable  sCallbackTable_;

         typedef std::map<std::string, int> ColorNameTable;
         ColorNameTable       colourTable_;

         bool                 enabled_;

         mutable RecursiveMutex mutex_;
   };

   template <>
   inline bool Settings::Get<bool>(std::string setting) const
   {
      return GetValue(setting, toggleTable_);
   }

   template <>
   inline std::string Settings::Get<std::string>(std::string setting) const
   {
      return GetValue(setting, stringTable_);
   }
}

#endif
/* vim: set sw=3 ts=3: */
