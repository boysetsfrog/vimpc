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

   commands.cpp - ex mode input handling 
   */

#include "commands.hpp"

#include <sstream>
#include <algorithm>
#include <iostream>

#include "console.hpp"
#include "dbc.hpp"
#include "screen.hpp"
#include "settings.hpp"
#include "vimpc.hpp"

using namespace Ui;

char const CommandPrompt   = ':';

// COMMANDS
Commands::Commands(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings) :
   InputMode          (CommandPrompt, screen),
   Player             (screen, client),
   settings_          (settings),
   screen_            (screen),
   initTabCompletion_ (true)
{
   // \todo add :quit! to quit and force song stop ?
   // \todo find a away to add aliases to tab completion
   // \todo allow aliases to be multiple commands, ie alias quit! stop; quit
   // or something similar
   commandTable_["!mpc"]      = &Commands::Mpc;
   commandTable_["alias"]     = &Commands::Alias;
   commandTable_["clear"]     = &Commands::ClearScreen;
   commandTable_["console"]   = &Commands::Console;
   commandTable_["connect"]   = &Commands::Connect;
   commandTable_["echo"]      = &Commands::Echo;
   commandTable_["help"]      = &Commands::Help;
   commandTable_["library"]   = &Commands::Library;
   commandTable_["next"]      = &Commands::Next;
   commandTable_["pause"]     = &Commands::Pause;
   commandTable_["play"]      = &Commands::Play;
   commandTable_["playlist"]  = &Commands::Playlist;
   commandTable_["previous"]  = &Commands::Previous;
   commandTable_["quit"]      = &Commands::Quit;
   commandTable_["random"]    = &Commands::Random;
   commandTable_["set"]       = &Commands::Set;
   commandTable_["stop"]      = &Commands::Stop;
}

Commands::~Commands()
{
}


void Commands::InitialiseMode()
{
   initTabCompletion_  = true;

   InputMode::InitialiseMode();
}

bool Commands::Handle(int const input)
{
   ResetTabCompletion(input);

   return InputMode::Handle(input);
}

void Commands::GenerateInputString(int input)
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
 
bool Commands::InputModeHandler(std::string input)
{
   return ExecuteCommand(inputString_);
}


bool Commands::ExecuteCommand(std::string const & input)
{
   bool result = true;

   std::string command, arguments;

   SplitCommand(input, command, arguments);

   if (aliasTable_.find(command) != aliasTable_.end())
   {
      std::string resolvedAlias(aliasTable_[command]);
      resolvedAlias.append(" ");
      resolvedAlias.append(arguments);

      result = ExecuteCommand(resolvedAlias);
   }
   else
   {
      result = ExecuteCommand(command, arguments);
   }

   return result;
}


bool Commands::ExecuteCommand(std::string const & command, std::string const & arguments)
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
      CommandTable::const_iterator it = commandTable_.find(commandToExecute);
      ptrToMember commandFunc = it->second;
      result = (*this.*commandFunc)(arguments);
   }

   return result;
}

void Commands::SplitCommand(std::string const & input, std::string & command, std::string & arguments)
{
   std::stringstream commandStream(input);
   std::getline(commandStream, command,   ' '); 
   std::getline(commandStream, arguments, '\n');
}

bool Commands::Set(std::string const & arguments)
{
   settings_.Set(arguments);

   // \todo work out wtf i am doing with these returns, seriously
   return true;
}

bool Commands::Mpc(std::string const & arguments)
{
   static uint32_t const bufferSize = 512;
   char   buf[bufferSize];

   std::string command("mpc ");
   command.append(arguments);

   screen_.ConsoleWindow().OutputLine("> %s", command.c_str());

   FILE * mpcOutput = popen(command.c_str(), "r");

   if (mpcOutput != NULL)
   {
      while (fgets(buf, bufferSize - 1, mpcOutput) != NULL)
      {
         screen_.ConsoleWindow().OutputLine("%s", buf);
      }

      pclose(mpcOutput);
   }

   // \todo
   return true;
}

bool Commands::Alias(std::string const & input)
{
   std::string command, arguments;

   SplitCommand(input, command, arguments);

   aliasTable_[command] = arguments;

   //\todo 
   return true;
}


void Commands::ResetTabCompletion(int input)
{
   if (input != '\t')
   {
      initTabCompletion_ = true;
   }
}


std::string Commands::TabComplete(std::string const & command)
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
