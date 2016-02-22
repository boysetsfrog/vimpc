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

   errorcodes.hpp - error numbers for each type of error that may occur
   */

#ifndef __ERRORCODES
#define __ERRORCODES

#include <string>

#define ERRORCODES \
   X(Invalid, "") \
   X(CommandAmbiguous,      "Command is ambiguous, please be more specific") \
   X(ClientError,           "MPD Error") \
   X(ClientNoMeta,          "Disabling database") \
   X(ClientNoConnection,    "Not Connected") \
   X(CommandNonexistant,    "Command not found") \
   X(CouldNotMapKeys,       "Failed to map the specified keys") \
   X(CouldNotParseKeys,     "Failed to parse the specified keys") \
   X(TabDoesNotExist,       "No such tab/window") \
   X(ExternalProgramError,  "Executing external program failed") \
   X(FindNoResults,         "Find: no results matching this pattern found") \
   X(HelpFileNonexistant,   "Unable to open help file") \
   X(NoParameter,           "Expected a parameter") \
   X(InvalidParameter,      "Invalid parameter") \
   X(NoPlaylistLoaded,      "Parameter missing or no playlist loaded") \
   X(UnknownOption,         "Unknown Option") \
   X(NoOutput,              "No such output") \
   X(PlaylistEmpty,         "Playlist: empty") \
   X(SearchNoResults,       "Pattern not found") \
   X(SettingNonexistant,    "No such setting") \
   X(PlaylistExists,        "Playlist with that name already exists") \
   X(NoSuchMark,            "Mark not set") \
   X(NoSuchMapping,         "No mapping found") \
   X(NameInUse,             "Name already in use") \
   X(NotSet,                "Required setting not set") \
   X(FileNotFound,          "File not found") \
   X(NoRangeAllowed,        "No range allowed for command") \
   X(ErrorClear,            "Clear all other errors") \
   X(WindowDisabled,        "Window not supported and is disabled") \
   X(Unknown,               "Unknown")

namespace ErrorNumber
{
   enum
   {
#define X(Number, String) Number,
      ERRORCODES
#undef X
      ErrorCount
   };
}

class ErrorStrings
{
public:
   static std::string Default[];
};

#endif
/* vim: set sw=3 ts=3: */
