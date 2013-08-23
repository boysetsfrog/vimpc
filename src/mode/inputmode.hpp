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

   inputmode.hpp - handles all input modes requiring a full string
   */

#ifndef __UI__INPUTMODE
#define __UI__INPUTMODE

#include "mode.hpp"

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

namespace Ui
{
   class ModeWindow;
   class Screen;

   static int const PromptSize = 1;

   // Cursor input mode
   class Cursor
   {
   public:
      Cursor(std::wstring & inputWString);
      ~Cursor();

   public:
      typedef enum
      {
         CursorNoMovement,
         CursorLeft,
         CursorRight,
         CursorStart,
         CursorEnd,
         CursorPreviousSpace,
         CursorNextSpace
      } CursorState;

      uint16_t Position() const;
      uint16_t DisplayPosition() const;
      uint16_t UpdatePosition(CursorState newCursorState);
      void     SetPosition(uint16_t position);
      void     ResetCursorPosition();

   private:
      uint16_t LimitCursorPosition(uint16_t position) const;

   private:
      int            input_;
      uint16_t       position_;
      std::wstring & inputWString_;
   };


   // Handles all input received whilst in a line input mode
   class InputMode : public Mode
   {
   private:
      typedef void (Ui::InputMode::*InputFunction)();

   public:
      InputMode(Ui::Screen & screen);
      virtual ~InputMode();

   private:
      InputMode(InputMode & inputMode);
      InputMode & operator=(InputMode & inputMode);

   public: // Ui::Mode
      void Initialise(int input);
      void Finalise(int input);
      void Refresh();
      bool Handle(int input);
      bool CausesModeToStart(int input) const;
      bool CausesModeToEnd(int input) const;

   public:
      static std::string SplitStringAtTerminator(std::string input, bool keepTerminator = true);

      bool SetInputString(std::string input);
      bool HasCompleteInput(int input) const;

      virtual void GenerateInputString(int input);
      virtual bool InputStringHandler(std::string input) = 0;

   private:
      virtual char const * Prompt() const = 0;

   protected:
      bool RequireDeletion(int input) const;

   private:
      bool RequireHistorySearch(int input) const;
      void ResetHistory(int input);
      void AddToHistory(std::string const & inputString);
      void InitialiseHistorySearch(std::string const & inputString);

      // Search up/down in the history, for the previously entered input
      // until we reach the top/bottom of the stack
      typedef enum
      {
         Up,
         Down
      } Direction;
      std::string SearchHistory(Direction direction, std::string const & inputString);

   public:
      static bool InputIsValidCharacter(int input);

   protected:
      std::wstring stringtow(std::string & string);
      std::string wtostring(std::wstring & string);

   private:
      template <Cursor::CursorState State>
      void MoveCursor();

      template <Direction Dir>
      void SearchHistory();

      template <Cursor::CursorState State>
      void Deletion();

      void ClearBeforeCursor();
      void ClearWordBeforeCursor();

   protected:
      std::wstring     inputWString_;
      std::string      inputString_;
      std::string      currentInput_;
      bool             backedOut_;

   private:
      typedef std::vector<std::string> History;
      typedef std::map<int, InputFunction> InputTable;

   private:
      ModeWindow     * window_;
      Cursor           cursor_;
      Ui::Screen     & screen_;
      bool             initHistorySearch_;
      bool             saveToHistory_;
      History          history_;
      History          searchHistory_;
      InputTable       inputTable_;

      // Class used to search through the history table
      class HistoryNotCompletionMatch
      {
      public:
         HistoryNotCompletionMatch(std::string const & key) :
            key_(key)
         {}

      public:
         bool operator() (std::string const & inputString)
         {
            return !(key_.compare(inputString.substr(0, key_.length())) == 0);
         }

      private:
         std::string key_;
      };
   };
}

#endif
/* vim: set sw=3 ts=3: */
