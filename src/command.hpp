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

   commands.hpp - ex mode input handling 
   */

#ifndef __UI__COMMAND
#define __UI__COMMAND

#include <string>
#include <map>
#include <vector>

#include "handler.hpp"
#include "inputmode.hpp"
#include "modewindow.hpp"
#include "player.hpp"

namespace Main
{
   class Settings;
   class Vimpc;
}

namespace Ui
{
   class Screen;

   // Handles all input received whilst in command mode
   class Command : public InputMode, public Player
   {

   public:
      Command(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings);
      ~Command();

   public: // Ui::Handler Inherits
      void InitialiseMode(int input);
      bool Handle(int input);

   private:
      //
      void GenerateInputString(int const input);

      char const * const Prompt();
      bool InputModeHandler(std::string input);

   public:
      // Checks if there is any aliases defined for a command, recursively calling
      // until a proper command is found then executes that command
      //
      // \param[in] input The command string to execute, including the arguments
      bool ExecuteCommand(std::string const & input);


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

      // Redraws the current window
      bool Redraw(std::string const & arguments);

      //
      void ResetTabCompletion(int input);

      // Complete the current command, by searching through the command table
      // for commands that begin with the currently set command
      std::string TabComplete(std::string const & command);

   public:
      typedef bool (Ui::Command::*ptrToMember)(std::string const &);

   private:
      typedef std::map<std::string, std::string> AliasTable;
      typedef std::map<std::string, ptrToMember> CommandTable;

   private: 
      bool             initTabCompletion_;
      AliasTable       aliasTable_;
      CommandTable     commandTable_;

      Main::Settings & settings_;
      Ui::Screen     & screen_;

      // Tab completion searching class
      class TabCompletionMatch 
      {
      public:
         TabCompletionMatch(std::string const & key) : 
            key_(key) 
         {}

      public:
         bool operator() (std::pair<std::string const &, Ui::Command::ptrToMember> element) 
         {
            std::string input(element.first);
            return (key_.compare(input.substr(0, key_.length())) == 0);
         }

      private:
         std::string key_;
      };
   };
}

#endif
