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

   search.cpp - handles searching the current window
   */

#include "search.hpp"

#include "algorithm.hpp"
#include "settings.hpp"
#include "vimpc.hpp"
#include "buffer/playlist.hpp"
#include "window/error.hpp"

#include <iostream>
#include <pcrecpp.h>

using namespace Ui;

Search::Search(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings) :
   InputMode   (screen),
   direction_  (Forwards),
   currentLine_(-1),
   lastSearch_ (""),
   hasSearched_(false),
   highlight_  (true),
   prompt_     (),
   settings_   (settings),
   screen_     (screen)
{
   prompt_[Forwards]  = '/';
   prompt_[Backwards] = '?';
}

Search::~Search()
{
}


void Search::Initialise(int input)
{
   highlight_   = false;
   direction_   = GetDirectionForInput(input);
   currentLine_ = screen_.ActiveWindow().CurrentLine();

   InputMode::Initialise(input);
}

void Search::Finalise(int input)
{
   highlight_ = true;

   if ((lastSearch_ != inputString_) || (hasSearched_ == false))
   {
      screen_.ScrollTo(currentLine_);
   }

   InputMode::Finalise(input);
}

bool Search::Handle(int input)
{
   static bool LastIncFound  = true;
   static std::string Match  = "";
   bool        Result        = true;

   if (settings_.Get(Setting::IncrementalSearch) == false)
   {
      Result = InputMode::Handle(input);
      LastIncFound = true;
      Match        = "";
   }
   else
   {
      if (HasCompleteInput(input) == true)
      {
         lastSearch_  = inputString_;
         hasSearched_ = true;
         Result = SearchResult(Next, inputString_, currentLine_, 1);
         LastIncFound = true;
         Match        = "";
      }
      // If we know there are no results, don't search, otherwise incsearch
      else
      {
         Result = InputMode::Handle(input);

         if ((inputString_ == Match) || (LastIncFound == true))
         {
            LastIncFound = SearchResult(Next, inputString_, currentLine_, 1, false);

            if (LastIncFound == true)
            {
               Match = inputString_;
            }
         }
         else 
         {
            LastIncFound = false;
         }
      }
   }

   return Result;
}


bool Search::CausesModeToStart(int input) const
{
   return ((input == prompt_[Forwards]) || (input == prompt_[Backwards]));
}

std::string Search::LastSearchString() const
{
   return StripFlags(lastSearch_);
}

pcrecpp::RE_Options Search::LastSearchOptions() const
{
   return GetOptions(lastSearch_);
}

bool Search::SearchResult(Skip skip, uint32_t count)
{
   (void) SearchResult(skip, lastSearch_, screen_.ActiveWindow().CurrentLine(), count);
   return true;
}

bool Search::SearchResult(Skip skip, std::string const & search, int32_t line, uint32_t count, bool raiseError)
{
   Direction direction = direction_;

   if (skip == Previous)
   {
      direction = SwapDirection(direction);
   }

   bool const found = SearchWindow(direction, search, line, count);

   if (found == false)
   {
      if (raiseError == true)
      {
         ErrorString(ErrorNumber::SearchNoResults, search);
      }
      else
      {
         screen_.ScrollTo(currentLine_);
      }
   }

   return found;
}

bool Search::SearchWindow(Direction direction, std::string search, int32_t startLine, uint32_t count)
{
   bool found = false;

   found = SearchForResult(direction, search, count, startLine);

   if ((found == false) && (settings_.Get(Setting::SearchWrap) == true))
   {
      if (direction == Forwards)
      {
         found = SearchForResult(direction, search, count, -1);
      }
      else
      {
         found = SearchForResult(direction, search, count, screen_.ActiveWindow().ContentSize() + 1);
      }
   }

   return found;
}


bool Search::SearchForResult(Direction direction, std::string search, uint32_t count, int32_t startLine)
{
   bool found = false;

   if (direction == Forwards)
   {
      for (int32_t i = startLine + 1; ((i <= static_cast<int32_t>(screen_.ActiveWindow().ContentSize())) && (found == false)); ++i)
      {
         found = CheckForMatch(search, i, count);
      }
   }
   else
   {
      for (int32_t i = startLine - 1; ((i >= 0) && (found == false)); --i)
      {
         found = CheckForMatch(search, i, count);
      }
   }

   return found;
}

Search::Direction Search::SwapDirection(Direction direction) const
{
   return ((direction == Forwards) ? Backwards : Forwards);
}

Search::Direction Search::GetDirectionForInput(int input) const
{
   Direction direction = Forwards;

   if (input == prompt_[Backwards])
   {
      direction = Backwards;
   }

   return direction;
}

bool Search::CheckForMatch(std::string const & search, int32_t songId, uint32_t & count)
{
   // see :help pattern-overview for full descriptions of what should be supported
   pcrecpp::RE   expression(".*" + StripFlags(search) + ".*", GetOptions(search));
   bool          found     (false);

   //std::string songDescription(screen_.PlaylistWindow().GetSong(songId)->PlaylistDescription());
   std::string searchPattern(screen_.ActiveWindow().SearchPattern(songId));

   if (expression.FullMatch(searchPattern.c_str()) == true)
   {
      screen_.ScrollTo(songId);

      --count;
      found = (count == 0);
   }

   return found;
}

pcrecpp::RE_Options Search::GetOptions(const std::string & search) const
{
   pcrecpp::RE_Options opt;

   if ((settings_.Get(Setting::IgnoreCaseSearch) == true) && (settings_.Get(Setting::SmartCase) == true))
   {
      if (Algorithm::isLower(search) == true)
      {
         opt.set_caseless(true);
      }
   }
   else if (settings_.Get(Setting::IgnoreCaseSearch) == true)
   {
      opt.set_caseless(true);
   }

   if (search.find("\\c") != string::npos)
   {
      opt.set_caseless(true);
   }
   else if (search.find("\\C") != string::npos)
   {
      opt.set_caseless(false);
   }

   return opt;
}

std::string Search::StripFlags(std::string search) const
{
   std::string Result(search);

   size_t found;

   //! \todo this is a hack we should really just loop
   //! over the input once and remove all flags
   while ((found = Result.find("\\c")) != string::npos)
   {
      Result.erase(found, 2);
   }

   while ((found = Result.find("\\C")) != string::npos)
   {
      Result.erase(found, 2);
   }

   return Result;
}

char const * Search::Prompt() const
{
   static char SearchPrompt[PromptSize + 1] = "";

   SearchPrompt[0] = prompt_[direction_];
   SearchPrompt[1] = '\0';

   return SearchPrompt;
}

bool Search::InputStringHandler(std::string input)
{
   lastSearch_  = input;
   hasSearched_ = true;
   return SearchResult(Next, 1);
}
/* vim: set sw=3 ts=3: */
