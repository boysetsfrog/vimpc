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

   command.cpp - ex mode input handling 
   */

#include "command.hpp"

#include <sstream>
#include <algorithm>
#include <iostream>

#include "assert.hpp"
#include "console.hpp"
#include "settings.hpp"
#include "vimpc.hpp"

using namespace Ui;

// COMMANDS
Command::Command(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings) :
   InputMode           (screen),
   Player              (screen, client, settings),
   initTabCompletion_  (true),
   aliasTable_         (),
   commandTable_       (),
   settings_           (settings),
   screen_             (screen)
{
   // \todo add :quit! to quit and force song stop ?
   // \todo find a away to add aliases to tab completion
   // \todo allow aliases to be multiple commands, ie alias quit! stop; quit
   // or something similar
   commandTable_["!mpc"]      = &Command::Mpc;
   commandTable_["alias"]     = &Command::Alias;
   commandTable_["clear"]     = &Command::ClearScreen;
   commandTable_["connect"]   = &Command::Connect;
   commandTable_["echo"]      = &Command::Echo;
   commandTable_["pause"]     = &Command::Pause;
   commandTable_["play"]      = &Command::Play;
   commandTable_["quit"]      = &Command::Quit;
   commandTable_["random"]    = &Command::Random;
   commandTable_["redraw"]    = &Command::Redraw;
   commandTable_["set"]       = &Command::Set;
   commandTable_["stop"]      = &Command::Stop;

   commandTable_["next"]      = &Command::SkipSong<Player::Next>;
   commandTable_["previous"]  = &Command::SkipSong<Player::Previous>;

   commandTable_["console"]   = &Command::SetActiveWindow<Ui::Screen::Console>;
   commandTable_["help"]      = &Command::SetActiveWindow<Ui::Screen::Help>;
   commandTable_["library"]   = &Command::SetActiveWindow<Ui::Screen::Library>;
   commandTable_["playlist"]  = &Command::SetActiveWindow<Ui::Screen::Playlist>;
}

Command::~Command()
{
}


void Command::InitialiseMode(int input)
{
   initTabCompletion_  = true;

   InputMode::InitialiseMode(input);
}

bool Command::Handle(int const input)
{
   ResetTabCompletion(input);

   return InputMode::Handle(input);
}

void Command::GenerateInputString(int input)
{
   if (input == '\t')
   {
      inputString_ = TabComplete(inputString_);
   }
   else
   {
      InputMode::GenerateInputString(input);
   }
}
 

bool Command::ExecuteCommand(std::string const & input)
{
   bool        result = true;
   std::string command, arguments;

   SplitCommand(input, command, arguments);

   if (aliasTable_.find(command) != aliasTable_.end())
   {
      std::string const resolvedAlias(aliasTable_[command] + " " + arguments);

      result = ExecuteCommand(resolvedAlias);
   }
   else
   {
      result = ExecuteCommand(command, arguments);
   }

   return result;
}


char const * const Command::Prompt() const
{
   static char const CommandPrompt[] = ":";
   return CommandPrompt;
}

bool Command::InputStringHandler(std::string input)
{
   return ExecuteCommand(input);
}


bool Command::ExecuteCommand(std::string const & command, std::string const & arguments)
{
   std::string commandToExecute  = command;
   bool        result            = true;
   bool        matchingCommand   = (commandTable_.find(commandToExecute) != commandTable_.end());

   // If we can't find the exact command, look for a unique command that starts
   // with the input command
   if (matchingCommand == false)
   {
      uint32_t validCommandCount = 0;

      for (CommandTable::const_iterator it = commandTable_.begin(); it != commandTable_.end(); ++it)
      {
         if (command.compare(it->first.substr(0, command.length())) == 0)
         {
            ++validCommandCount;
            commandToExecute = it->first;
         }
      }

      matchingCommand = (validCommandCount == 1);
   }

   // If we have found a command execute it, with \p arguments
   if (matchingCommand == true)
   {
      CommandTable::const_iterator const it = commandTable_.find(commandToExecute);
      CommandFunction const commandFunction = it->second;

      result = (*this.*commandFunction)(arguments);
   }

   return result;
}

void Command::SplitCommand(std::string const & input, std::string & command, std::string & arguments)
{
   std::stringstream commandStream(input);
   std::getline(commandStream, command,   ' '); 
   std::getline(commandStream, arguments, '\n');
}

bool Command::Set(std::string const & arguments)
{
   settings_.Set(arguments);
   return true;
}

bool Command::Mpc(std::string const & arguments)
{
   static uint32_t const bufferSize = 512;
   char   buffer[bufferSize];

   std::string const command("mpc " + arguments);

   screen_.ConsoleWindow().OutputLine("> %s", command.c_str());

   FILE * const mpcOutput = popen(command.c_str(), "r");

   if (mpcOutput != NULL)
   {
      while (fgets(buffer, bufferSize - 1, mpcOutput) != NULL)
      {
         screen_.ConsoleWindow().OutputLine("%s", buffer);
      }

      pclose(mpcOutput);
   }

   return true;
}

bool Command::Alias(std::string const & input)
{
   std::string command, arguments;

   SplitCommand(input, command, arguments);

   aliasTable_[command] = arguments;

   return true;
}

bool Command::Redraw(std::string const & arguments)
{
   screen_.Redraw();
   return true;
}

void Command::ResetTabCompletion(int input)
{
   if (input != '\t')
   {
      initTabCompletion_ = true;
   }
}


std::string Command::TabComplete(std::string const & command)
{
   static std::string tabStart(command);
   static CommandTable::iterator tabIterator(commandTable_.begin());

   std::string result;

   if (initTabCompletion_ == true)
   {
      initTabCompletion_ = false;
      tabIterator        = commandTable_.begin();
      tabStart           = command;
   }

   tabIterator = find_if(tabIterator, commandTable_.end(), TabCompletionMatch(tabStart));

   if (tabIterator != commandTable_.end())
   {
      result = tabIterator->first;
      ++tabIterator;
   }
   else
   {
      initTabCompletion_ = true;
      result             = tabStart;
   }

   return result;
}
