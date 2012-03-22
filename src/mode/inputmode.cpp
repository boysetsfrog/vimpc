/*
   Vimpc
   Copyright (C) 2010 Nathan Sweetman

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

   inputmode.cpp - handles all input modes requiring a full string
   */

#include "inputmode.hpp"

#include <limits>
#include <algorithm>

#include "assert.hpp"
#include "screen.hpp"
#include "window/console.hpp"

using namespace Ui;


InputMode::InputMode(Ui::Screen & screen) :
   inputString_       (""),
   backedOut_         (false),
   window_            (NULL),
   cursor_            (inputString_),
   screen_            (screen),
   initHistorySearch_ (true),
   history_           (),
   searchHistory_     ()
{
}

InputMode::~InputMode()
{
   screen_.DeleteModeWindow(window_);
   window_ = NULL;
}


void InputMode::Initialise(int input)
{
   // \todo this should really be in the constructor
   // but that breaks the search at the moment as the screen is constructed
   // after the search... fix this
   if (window_ == NULL)
   {
      window_ = screen_.CreateModeWindow();
   }

   initHistorySearch_  = true;

   inputString_.clear();

   cursor_.ResetCursorPosition();
   window_->ShowCursor();
   window_->SetCursorPosition(cursor_.Position());
   window_->SetLine(Prompt());
   Refresh();

   ENSURE(inputString_.empty() == true);
}

void InputMode::Finalise(int input)
{
   AddToHistory(inputString_);
   window_->HideCursor();
   Refresh();
}

void InputMode::Refresh()
{
   window_->Print(0);
}

bool InputMode::Handle(int const input)
{
   bool result = true;

   if (HasCompleteInput(input) == true)
   {
      result = InputStringHandler(inputString_);
      screen_.Update();
   }
   else
   {
      ResetHistory(input);
      GenerateInputString(input);
      window_->SetCursorPosition(cursor_.UpdatePosition(input));
      window_->SetLine("%s%s", Prompt(), inputString_.c_str());
   }

   return result;
}

bool InputMode::CausesModeToStart(int input) const
{
   return (static_cast<char>(input) == Prompt()[0]);
}

bool InputMode::CausesModeToEnd(int input) const
{
   return (((input == KEY_BACKSPACE) || (input == 0x7F)) && (inputString_ == "") && (backedOut_));
}


/* static */ std::string InputMode::SplitStringAtTerminator(std::string input)
{
   std::vector<std::string> terminators;

   terminators.push_back("\n");
   terminators.push_back("<C-M>");
   terminators.push_back("<Enter>");
   terminators.push_back("<Return>");

   for (int i = 0; i < input.length(); ++i)
   {
      for (std::vector<std::string>::iterator it = terminators.begin(); it != terminators.end(); ++it)
      {
         if (input.substr(i, (*it).length()) == (*it))
         {
            return input.substr(0, i + (*it).length());
         }
      }
   }

   return input;
}

/* static */ std::string InputMode::RemoveStringTerminator(std::string input)
{
   std::vector<std::string> terminators;

   terminators.push_back("\n");
   terminators.push_back("<C-M>");
   terminators.push_back("<Enter>");
   terminators.push_back("<Return>");

   for (int i = 0; i < input.length(); ++i)
   {
      for (std::vector<std::string>::iterator it = terminators.begin(); it != terminators.end(); ++it)
      {
         if (input.substr(i, (*it).length()) == (*it))
         {
            return input.substr(0, i);
         }
      }
   }

   return input;
}

bool InputMode::SetInputString(std::string input)
{
   inputString_ = RemoveStringTerminator(input);

   if (inputString_ != input)
   {
      Handle('\n');
      return true;
   }
   else
   {
      window_->SetCursorPosition(inputString_.length() + PromptSize);
      window_->SetLine(std::string(Prompt()) + inputString_);
   }

   return false;
}

bool InputMode::HasCompleteInput(int input)
{
   return (input == '\n');
}

void InputMode::GenerateInputString(int input)
{
   // \todo this could use a refactor
   int64_t const cursorPosition = (cursor_.Position() - PromptSize);

   if (RequireHistorySearch(input) == true)
   {
      Direction const direction = (input == KEY_UP) ? Up : Down;

      inputString_ = SearchHistory(direction, inputString_);
   }
   else if (RequireDeletion(input) == true)
   {
      if (inputString_.empty() == false)
      {
         backedOut_ = false;
         const int cursorMovement = ((input == KEY_BACKSPACE) || (input == 0x7F)) ? 1 : 0;

         if ((cursorPosition - cursorMovement) >= 0)
         {
            inputString_.erase((cursorPosition - cursorMovement), 1);
         }
      }
      else
      {
         backedOut_ = true;
      }
   }
   if (InputIsValidCharacter(input) == true)
   {
      inputString_.insert(static_cast<std::string::size_type>(cursorPosition), 1, static_cast<char>(input));
   }
}


bool InputMode::RequireDeletion(int input) const
{
   return (((input == KEY_BACKSPACE) || (input == 0x7F) || (input == KEY_DC)));
}


bool InputMode::RequireHistorySearch(int input) const
{
   return ((input == KEY_UP) || (input == KEY_DOWN));
}

void InputMode::ResetHistory(int input)
{
   if (RequireHistorySearch(input) == false)
   {
      initHistorySearch_ = true;
   }
}

void InputMode::AddToHistory(std::string const & inputString)
{
   if (inputString.empty() == false)
   {
      //Remove if it is already in the history
      //Then add it to the back (as the most recent input)
      history_.erase(std::remove(history_.begin(), history_.end(), inputString), history_.end());
      history_.push_back(inputString);
   }
}

void InputMode::InitialiseHistorySearch(std::string const & inputString)
{
   searchHistory_.clear();
   searchHistory_.reserve(history_.size());

   std::remove_copy_if(history_.begin(), history_.end(), std::back_inserter(searchHistory_),
                       HistoryNotCompletionMatch(inputString));
}

std::string InputMode::SearchHistory(Direction direction, std::string const & inputString)
{
   static History::reverse_iterator historyIterator;
   static std::string historyLastResult(inputString);
   static std::string historyStart     (inputString);

   std::string result;

   if (initHistorySearch_ == true)
   {
      initHistorySearch_   = false;
      historyStart         = inputString;

      InitialiseHistorySearch(historyStart);

      historyIterator = searchHistory_.rbegin();
      --historyIterator;
   }

   if (historyIterator == searchHistory_.rend())
   {
      --historyIterator;
   }

   if (direction == Up)
   {
      if (historyIterator < searchHistory_.rend())
      {
         ++historyIterator;
      }

      result = ((historyIterator == searchHistory_.rend()) ? historyLastResult : *historyIterator);
   }
   else
   {
      if (historyIterator >= searchHistory_.rbegin())
      {
         --historyIterator;
      }

      result = ((historyIterator < searchHistory_.rbegin()) ? historyStart : *historyIterator);
   }

   historyLastResult = result;

   return result;
}


bool InputMode::InputIsValidCharacter(int input)
{
   return (input < std::numeric_limits<char>::max())
       && (input != 27)
       && (input != '\n');
}


// Helper Classes
// CURSOR
Cursor::Cursor(std::string & inputString) :
   input_       (0),
   position_    (PromptSize),
   inputString_ (inputString)
{
}

Cursor::~Cursor()
{
}

uint16_t Cursor::Position() const
{
   return position_;
}

uint16_t Cursor::UpdatePosition(int input)
{
   uint16_t const minCursorPosition = PromptSize;
   uint16_t const maxCursorPosition = (inputString_.size() + PromptSize);

   input_ = input;

   CursorState newCursorState = CursorMovementState();

   //Determine where the cursor should move to
   switch (newCursorState)
   {
      case CursorLeft:
         --position_;
         break;

      case CursorRight:
         ++position_;
         break;

      case CursorStart:
         position_ = minCursorPosition;
         break;

      case CursorEnd:
         position_ = maxCursorPosition;
         break;

      case CursorNoMovement:
         break;

      default:
         ASSERT(false);
         break;
   }

   position_ = LimitCursorPosition(position_);

   return position_;
}

void Cursor::ResetCursorPosition()
{
   position_ = PromptSize;
}

bool Cursor::WantCursorLeft() const
{
   return ((input_ == KEY_LEFT) ||
           (input_ == KEY_BACKSPACE) ||
           (input_ == 0x7F));
}

bool Cursor::WantCursorEnd() const
{
   return ((input_ == KEY_UP)   ||
           (input_ == KEY_DOWN) ||
           (input_ == '\t'));
}

bool Cursor::WantCursorRight() const
{
   return ((input_ == KEY_RIGHT) ||
           (InputMode::InputIsValidCharacter(input_) == true));
}

bool Cursor::WantCursorStart() const
{
   return ((input_ == KEY_HOME));
}

Cursor::CursorState Cursor::CursorMovementState()
{
   CursorState newCursorState = CursorNoMovement;

   if (WantCursorStart() == true)
   {
      newCursorState = CursorStart;
   }
   else if (WantCursorEnd() == true)
   {
      newCursorState = CursorEnd;
   }
   else if (WantCursorLeft() == true)
   {
      newCursorState = CursorLeft;
   }
   else if (WantCursorRight() == true)
   {
      newCursorState = CursorRight;
   }

   return newCursorState;
}

uint16_t Cursor::LimitCursorPosition(uint16_t position) const
{
   uint16_t const minCursorPosition = PromptSize;
   uint16_t const maxCursorPosition = (inputString_.size() + PromptSize);

   //Ensure that the cursor is in a valid range
   if (position_ < minCursorPosition)
   {
      position = minCursorPosition;
   }
   else if (position_ > maxCursorPosition)
   {
      position = maxCursorPosition;
   }

   return position;
}
/* vim: set sw=3 ts=3: */
