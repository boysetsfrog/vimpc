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
#include "algorithm.hpp"
#include "screen.hpp"
#include "window/console.hpp"
#include "window/debug.hpp"

using namespace Ui;

#define ESCAPE_KEY 27

InputMode::InputMode(Ui::Screen & screen) :
   inputString_       (),
   backedOut_         (false),
   window_            (NULL),
   cursor_            (inputWString_),
   screen_            (screen),
   initHistorySearch_ (true),
   saveToHistory_     (true),
   history_           (),
   searchHistory_     ()
{
   // Currently ctrl key combinations are 'X' - 'A' + 1 (for <C-X>)
   inputTable_[KEY_UP]        = &InputMode::SearchHistory<Up>;
   inputTable_['P' - 'A' + 1] = &InputMode::SearchHistory<Up>;
   inputTable_[KEY_DOWN]      = &InputMode::SearchHistory<Down>;
   inputTable_['N' - 'A' + 1] = &InputMode::SearchHistory<Down>;
   inputTable_[KEY_BACKSPACE] = &InputMode::Deletion<Cursor::CursorLeft>;
   inputTable_['H' - 'A' + 1] = &InputMode::Deletion<Cursor::CursorLeft>;
   inputTable_[0x7F]          = &InputMode::Deletion<Cursor::CursorLeft>;
   inputTable_[KEY_DC]        = &InputMode::Deletion<Cursor::CursorNoMovement>;
   inputTable_['\t']          = &InputMode::MoveCursor<Cursor::CursorEnd>;
   inputTable_[KEY_LEFT]      = &InputMode::MoveCursor<Cursor::CursorLeft>;
   inputTable_[KEY_RIGHT]     = &InputMode::MoveCursor<Cursor::CursorRight>;
   inputTable_[KEY_HOME]      = &InputMode::MoveCursor<Cursor::CursorStart>;
   inputTable_['A' - 'A' + 1] = &InputMode::MoveCursor<Cursor::CursorStart>;
   inputTable_['B' - 'A' + 1] = &InputMode::MoveCursor<Cursor::CursorStart>;
   inputTable_[KEY_END]       = &InputMode::MoveCursor<Cursor::CursorEnd>;
   inputTable_['E' - 'A' + 1] = &InputMode::MoveCursor<Cursor::CursorEnd>;
   inputTable_[KEY_SLEFT]     = &InputMode::MoveCursor<Cursor::CursorPreviousSpace>;
   inputTable_[KEY_SRIGHT]    = &InputMode::MoveCursor<Cursor::CursorNextSpace>;
   inputTable_['U' - 'A' + 1] = &InputMode::ClearBeforeCursor;
   inputTable_['W' - 'A' + 1] = &InputMode::ClearWordBeforeCursor;
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

   inputWString_.clear();
   inputString_.clear();
   currentInput_.clear();

   cursor_.ResetCursorPosition();
   window_->ShowCursor();
   window_->SetLine(Prompt());
   window_->SetCursorPosition(cursor_.DisplayPosition());

   ENSURE(inputString_.empty() == true);
}

void InputMode::Finalise(int input)
{
   if (saveToHistory_ == true)
   {
      AddToHistory(inputString_);
   }

   window_->HideCursor();
}

void InputMode::Refresh()
{
   screen_.PrintModeWindow(window_);
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
      saveToHistory_ = true;
      ResetHistory(input);
      GenerateInputString(input);
      window_->SetLine("%s%s", Prompt(), inputString_.c_str());
      window_->SetCursorPosition(cursor_.DisplayPosition());
   }

   return result;
}

bool InputMode::CausesModeToStart(int input) const
{
   return (static_cast<char>(input) == Prompt()[0]);
}

bool InputMode::CausesModeToEnd(int input) const
{
   return ((((input == KEY_BACKSPACE) || (input == 0x7F)) && (inputString_.empty() == true) && (backedOut_)) ||
           (input == ESCAPE_KEY) ||
           (HasCompleteInput(input)));
}


/* static */ std::string InputMode::SplitStringAtTerminator(std::string input, bool keepTerminator)
{
   static std::vector<std::string> terminators;

   if (terminators.empty() == true)
   {
      terminators.push_back("\n");
      terminators.push_back("<C-M>");
      terminators.push_back("<Enter>");
      terminators.push_back("<Return>");
      terminators.push_back("<CR>");
   }

   for (uint32_t i = 0; i < input.length(); ++i)
   {
      for (auto terminator : terminators)
      {
         std::string text = input.substr(i, terminator.length());

         if (Algorithm::iequals(text, terminator) == true)
         {
            return input.substr(0, i + ((keepTerminator == true) ? terminator.length() : 0));
         }
      }
   }

   return input;
}

bool InputMode::SetInputString(std::string input)
{
   inputString_  = SplitStringAtTerminator(input, false);
   inputWString_ = stringtow(inputString_);

   if (inputString_ != input)
   {
      saveToHistory_ = false;
      Handle('\n');
      return true;
   }
   else
   {
      cursor_.SetPosition(inputWString_.length() + PromptSize);
      window_->SetLine(std::string(Prompt()) + inputString_);
      window_->SetCursorPosition(cursor_.DisplayPosition());
   }

   return false;
}

bool InputMode::HasCompleteInput(int input) const
{
   return (input == '\n');
}

void InputMode::GenerateInputString(int input)
{
   if (inputTable_.find(input) != inputTable_.end())
   {
      InputFunction func = inputTable_[input];
      (*this.*func)();
   }
   else if (InputIsValidCharacter(input) == true)
   {
      int64_t const cursorPosition = (cursor_.Position() - PromptSize);

      if (((static_cast<char>(input) & 0xc0)) != 0x80)
      {
         currentInput_.clear();
      }

      wchar_t wide;
      currentInput_.append(1, static_cast<char>(input));
      mbtowc(NULL, NULL, 0);
      int length = mbtowc(&wide, currentInput_.c_str(), currentInput_.length());

      if (length > 0)
      {
         inputWString_.insert(static_cast<std::wstring::size_type>(cursorPosition), 1, wide);
         inputString_ = wtostring(inputWString_);
         cursor_.UpdatePosition(Cursor::CursorRight);
         currentInput_.clear();
      }
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
   return (input < std::numeric_limits<unsigned char>::max()) &&
          (input != ESCAPE_KEY) &&
          (input != '\n');
}


template <Cursor::CursorState State>
void InputMode::MoveCursor()
{
   cursor_.UpdatePosition(State);
}

template <InputMode::Direction Dir>
void InputMode::SearchHistory()
{
   inputString_  = SearchHistory(Dir, inputString_);
   inputWString_ = stringtow(inputString_);
   cursor_.UpdatePosition(Cursor::CursorEnd);
}

template <Cursor::CursorState State>
void InputMode::Deletion()
{
   int64_t const cursorPosition = (cursor_.Position() - PromptSize);

   if (inputString_.empty() == false)
   {
      int const cursorMovement = (State == Cursor::CursorLeft) ? 1 : 0;

      if ((cursorPosition - cursorMovement) >= 0)
      {
         inputWString_.erase((cursorPosition - cursorMovement), 1);
         inputString_ = wtostring(inputWString_);
      }

      cursor_.UpdatePosition(State);

      backedOut_ = false;
   }
   else
   {
      backedOut_ = true;
   }
}

void InputMode::ClearBeforeCursor()
{
   inputWString_.erase(0, cursor_.Position() - 1);
   inputString_ = wtostring(inputWString_);
   cursor_.UpdatePosition(Cursor::CursorStart);
}

void InputMode::ClearWordBeforeCursor()
{
   int previousSpace = inputWString_.find_last_of(' ', cursor_.Position() - 2);

   if (previousSpace != -1)
   {
      inputWString_.erase(previousSpace, cursor_.Position() - (1 + previousSpace));
      inputString_ = wtostring(inputWString_);
      cursor_.SetPosition(1 + previousSpace);
   }
   else
   {
      ClearBeforeCursor();
   }
}

std::wstring InputMode::stringtow(std::string & string)
{
   std::wstring result = L"";

   if (string.length() > 0)
   {
      wchar_t * wbuffer = new wchar_t[string.length() + 1];
      size_t const mblength = mbstowcs(wbuffer, string.c_str(), string.length());

      if (mblength != static_cast<size_t>(-1))
      {
         result = std::wstring(wbuffer, mblength);
      }

      delete[] wbuffer;
   }

   return result;
}

std::string InputMode::wtostring(std::wstring & string)
{
   if (string.length() > 0)
   {
      char * buffer = new char[string.length() * sizeof(wchar_t) + 1];
      wcstombs(buffer, string.c_str(), string.length() * sizeof(wchar_t));
      std::string result = buffer;
      delete[] buffer;
      return result;
   }

   return "";
}


// Helper Classes
// CURSOR
Cursor::Cursor(std::wstring & inputWString) :
   input_       (0),
   position_    (PromptSize),
   inputWString_(inputWString)
{
}

Cursor::~Cursor()
{
}

uint16_t Cursor::Position() const
{
   return position_;
}

uint16_t Cursor::DisplayPosition() const
{
   int pos = 0;

   for (int i = 0; i < position_ - PromptSize; ++i)
   {
      pos += wcwidth(inputWString_[i]);
   }

   return pos + PromptSize;
}

uint16_t Cursor::UpdatePosition(CursorState newCursorState)
{
   uint16_t const minCursorPosition = PromptSize;
   uint16_t const maxCursorPosition = (inputWString_.size() + PromptSize);

   int previousSpace, nextSpace;

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

      case CursorPreviousSpace:
         previousSpace = inputWString_.find_last_of(' ', position_ - 2);
         position_ = previousSpace != -1 ? 1 + previousSpace : minCursorPosition;
         break;

      case CursorNextSpace:
         nextSpace = inputWString_.find_first_of(' ', position_);
         position_ = nextSpace != -1 ? 1 + nextSpace : maxCursorPosition;
         break;

      default:
         ASSERT(false);
         break;
   }

   position_ = LimitCursorPosition(position_);

   return position_;
}

void Cursor::SetPosition(uint16_t position)
{
   position_ = LimitCursorPosition(position);
}

void Cursor::ResetCursorPosition()
{
   position_ = PromptSize;
}

uint16_t Cursor::LimitCursorPosition(uint16_t position) const
{
   uint16_t const minCursorPosition = PromptSize;
   uint16_t const maxCursorPosition = (inputWString_.size() + PromptSize);

   //Ensure that the cursor is in a valid range
   if (position < minCursorPosition)
   {
      position = minCursorPosition;
   }
   else if (position > maxCursorPosition)
   {
      position = maxCursorPosition;
   }

   return position;
}
/* vim: set sw=3 ts=3: */
