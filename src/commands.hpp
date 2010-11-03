#ifndef __UI__COMMANDS
#define __UI__COMMANDS

#include <string>
#include <map>
#include <vector>

#include "handler.hpp"
#include "modewindow.hpp"
#include "player.hpp"

namespace Main
{
   class Settings;
   class Vimpc;
}

namespace Ui
{
   // Cursor for the command mode window
   class CommandCursor
   {
   public:
      CommandCursor(std::string & command);
      ~CommandCursor();

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
      std::string & command_;
   };
}


namespace Ui
{
   // Handles all input received whilst in command mode
   class Commands : public Handler, public Player
   {

   public:
      Commands(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings);
      ~Commands();

   public: // Ui::Handler Inherits
      void InitialiseMode();
      void FinaliseMode();
      bool Handle(int input);

   public:
      // Checks if there is any aliases defined for a command, recursively calling
      // until a proper command is found then executes that command
      //
      // \param[in] input The command string to execute, including the arguments
      bool ExecuteCommand(std::string const & input);

   public:
      static bool InputIsValidCharacter(int input);

   private:
      //
      bool HasCommandToExecute(int input) const;

      //
      void GenerateCommandFromInput(int const input);

      // Executes \p command using \p arguments
      //
      // \param[in] command   The command to execute
      // \param[in] arguments The arguments to pass to the command
      bool ExecuteCommand(std::string const & command, std::string const & arguments);

      // Splits the input into command and argument parts
      //
      // \param[in]  input     The string to split
      // \param[out] command   The command part of the string
      // \param[out] arguments The arguments from the string
      void SplitCommand(std::string const & input, std::string & command, std::string & arguments);

      // Handle the settings
      bool Set(std::string const & arguments);

      // Call the cli mpc client
      bool Mpc(std::string const & arguments);

      // Alias a command to a given string
      bool Alias(std::string const & arguments);

      //
      void ResetHistory(int input);
      void ResetTabCompletion(int input);

      //
      void AddCommandToHistory(std::string const & command);
      void InitialiseHistorySearch(std::string const & command);

      // Search up/down in the command history, for the previously executed command
      // until we reach the top/bottom of the command stack
      typedef enum
      {
         Up,
         Down
      } Direction;
      std::string History(Direction direction, std::string const & command);

      // Complete the current command, by searching through the command table
      // for commands that begin with the currently set command
      std::string TabComplete(std::string const & command);

   public:
      typedef bool (Ui::Commands::*ptrToMember)(std::string const &);

   private: 
      Main::Settings & settings_;
      ModeWindow     * window_;
      CommandCursor    cursor_;

      std::string      command_;
      bool             initHistorySearch_;
      bool             initTabCompletion_;

      //Tables
      typedef std::map<std::string, std::string> AliasTable;
      typedef std::map<std::string, ptrToMember> CommandTable;
      typedef std::vector<std::string> CommandHistory;

      AliasTable     aliasTable_;
      CommandTable   commandTable_;
      CommandHistory commandHistory_;
      CommandHistory searchCommandHistory_;


      // Command completion classes
      class TabCompletionMatch 
      {
      public:
         TabCompletionMatch(std::string const & key) : 
            key_(key) 
         {}

      public:
         bool operator() (std::pair<std::string const &, Ui::Commands::ptrToMember> element) 
         {
            std::string input(element.first);
            return (key_.compare(input.substr(0, key_.length())) == 0);
         }

      private:
         std::string key_;
      };

      class HistoryNotCompletionMatch 
      {
      public:
         HistoryNotCompletionMatch(std::string const & key) : 
            key_(key) 
         {}

      public:
         bool operator() (std::string const & command) 
         {
            return !(key_.compare(command.substr(0, key_.length())) == 0);
         }

      private:
         std::string key_;
      };
  };
}

#endif
