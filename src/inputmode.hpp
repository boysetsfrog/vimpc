#ifndef __UI__INPUTMODE
#define __UI__INPUTMODE

#include <string>
#include <map>
#include <vector>

#include "handler.hpp"
#include "modewindow.hpp"

namespace Ui
{
   class Screen;

   static int const PromptSize = 1;

   // Cursor input mode
   class Cursor
   {
   public:
      Cursor(std::string & inputString);
      ~Cursor();

   public:
      uint16_t Position();
      uint16_t UpdatePosition(int input);
      void     ResetCursorPosition();

   private:
      typedef enum
      {
         CursorNoMovement,
         CursorLeft,
         CursorRight,
         CursorStart,
         CursorEnd
      } CursorState;

   private:
      CursorState CursorMovementState();
      uint16_t LimitCursorPosition(uint16_t position) const;
      bool WantCursorLeft();
      bool WantCursorRight();
      bool WantCursorStart();
      bool WantCursorEnd();

   private:
      int           input_;
      uint16_t      position_;
      std::string & inputString_;
   };


   // Handles all input received whilst in a line input mode
   class InputMode : public Handler
   {
   public:
      InputMode(char const prompt, Ui::Screen & screen);
      virtual ~InputMode();

   public: // Ui::Handler Inherits
      virtual void InitialiseMode();
      virtual void FinaliseMode();
      virtual bool Handle(int input);

   public:
      virtual bool HasCompleteInput(int input);
      virtual void GenerateInputString(int input);

   public:
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
      virtual bool InputModeHandler(std::string input) = 0;

   public:
      static bool InputIsValidCharacter(int input);

   protected:
      std::string      inputString_;

   private:
      typedef std::vector<std::string> History;

   private: 
      ModeWindow     * window_;
      char             prompt_[PromptSize + 1];
      Cursor           cursor_;
      Ui::Screen     & screen_;
      bool             initHistorySearch_;
      History          history_;
      History          searchHistory_;

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
