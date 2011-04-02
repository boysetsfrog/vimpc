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

#include <algorithm>
#include <boost/regex.hpp>
#include <sstream>

#include "assert.hpp"
#include "settings.hpp"
#include "vimpc.hpp"
#include "window/console.hpp"
#include "window/error.hpp"

using namespace Ui;

// COMMANDS
Command::Command(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings) :
   InputMode           (screen),
   Player              (screen, client, settings),
   initTabCompletion_  (true),
   forceCommand_       (false),
   aliasTable_         (),
   commandTable_       (),
   screen_             (screen),
   settings_           (settings)
{
   // \todo find a away to add aliases to tab completion
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
   commandTable_["tabfirst"]  = &Command::ChangeToWindow<First>;
   commandTable_["tablast"]   = &Command::ChangeToWindow<Last>;
   commandTable_["tabhide"]   = &Command::HideWindow;
   commandTable_["tabmove"]   = &Command::MoveWindow;

   commandTable_["rescan"]    = &Command::Rescan;
   commandTable_["update"]    = &Command::Update;

   commandTable_["next"]      = &Command::SkipSong<Player::Next>;
   commandTable_["previous"]  = &Command::SkipSong<Player::Previous>;

   commandTable_["console"]   = &Command::SetActiveAndVisible<Ui::Screen::Console>;
   commandTable_["help"]      = &Command::SetActiveAndVisible<Ui::Screen::Help>;
   commandTable_["library"]   = &Command::SetActiveAndVisible<Ui::Screen::Library>;
   commandTable_["playlist"]  = &Command::SetActiveAndVisible<Ui::Screen::Playlist>;
}

Command::~Command()
{
}


void Command::Initialise(int input)
{
   initTabCompletion_  = true;

   InputMode::Initialise(input);
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
      boost::regex const blankCommand("^\\s*$");
      boost::regex const multipleCommandAlias("^(\\s*([^;]+)\\s*;?).*$");
      std::string        resolvedAlias(aliasTable_[command] + " " + arguments);
      boost::cmatch      match;

      while (boost::regex_match(resolvedAlias.c_str(), match, multipleCommandAlias) == true)
      {
         std::string const matchString  (match[1].first, match[1].second);
         std::string const commandString(match[2].first, match[2].second);
         resolvedAlias = resolvedAlias.substr(matchString.size(), resolvedAlias.size());

         if (boost::regex_match(commandString, blankCommand) == false)
         {   
            result = ExecuteCommand(commandString);
         }
      }
   }
   else
   {
      result = ExecuteCommand(command, arguments);
   }

   return result;
}


char const * Command::Prompt() const
{
   static char const CommandPrompt[] = ":";
   return CommandPrompt;
}

bool Command::InputStringHandler(std::string input)
{
   return ExecuteCommand(input);
}


bool Command::ClearScreen(UNUSED std::string const & arguments) 
{ 
   return Player::ClearScreen();
}

bool Command::Connect(std::string const & arguments) 
{ 
   size_t pos = arguments.find_first_of(" ");
   std::string hostname = arguments.substr(0, pos);
   std::string port     = arguments.substr(pos + 1);

   return Player::Connect(hostname, std::atoi(port.c_str()));
}

bool Command::Pause(UNUSED std::string const & arguments)
{ 
   return Player::Pause(); 
}

bool Command::Play(std::string const & arguments)
{ 
   return Player::Play(atoi(arguments.c_str())); 
}

bool Command::Quit(UNUSED std::string const & arguments)
{ 
   if ((forceCommand_ == true) || (settings_.StopOnQuit() == true))
   {
      Player::Stop();
   }

   return Player::Quit();
}

bool Command::Random(std::string const & arguments)
{ 
   bool const value = (arguments.compare("on") == 0);
   return Player::SetRandom(value);
}

bool Command::Redraw(UNUSED std::string const & arguments)
{ 
   return Player::Redraw(); 
}

bool Command::Stop(UNUSED std::string const & arguments)
{ 
   return Player::Stop(); 
}

bool Command::Rescan(UNUSED std::string const & arguments)
{ 
   return Player::Rescan();
}

bool Command::Update(UNUSED std::string const & arguments)
{ 
   return Player::Update(); 
}


//Implementation of skipping functions
template <Ui::Player::Skip SKIP>
bool Command::SkipSong(std::string const & arguments)
{
   uint32_t count = atoi(arguments.c_str());
   count = (count == 0) ? 1 : count;
   return Player::SkipSong(SKIP, count);
}


//Implementation of window change function
template <Ui::Screen::MainWindow MAINWINDOW>
bool Command::SetActiveAndVisible(UNUSED std::string const & arguments)
{
   screen_.SetActiveAndVisible(MAINWINDOW);

   return true;
}

template <Command::Location LOCATION>
bool Command::ChangeToWindow(UNUSED std::string const & arguments)
{
   uint32_t active = 0;

   if (LOCATION == Last)
   {
      active = screen_.VisibleWindows() - 1;
   }

   screen_.SetActiveWindow(active);

   return true;
}

bool Command::HideWindow(UNUSED std::string const & arguments)
{
   screen_.SetVisible(screen_.GetActiveWindow(), false);
   return true;
}

bool Command::MoveWindow(std::string const & arguments)
{
   screen_.MoveWindow(atoi(arguments.c_str()));
   return true;
}


bool Command::ExecuteCommand(std::string command, std::string const & arguments)
{
   boost::regex const forceCheck("^.*!$");
   bool         result          = true;
   forceCommand_                = false;

   if (boost::regex_match(command, forceCheck))
   {
      forceCommand_  = true;
      command        = command.substr(0, command.length() - 1);
   }

   // If we can't find the exact command, look for a unique command that starts
   // with the input command
   std::string commandToExecute  = command;
   bool        matchingCommand   = (commandTable_.find(commandToExecute) != commandTable_.end());
   uint32_t    validCommandCount = 0;

   if (matchingCommand == false)
   {

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
   else if (validCommandCount > 1)
   {
      Error(ErrorNumber::CommandAmbiguous, "Command is ambigous, please be more specific: " + command);
   }
   else
   {
      Error(ErrorNumber::CommandNonexistant, "Command not found: " + command);
   }

   // \todo will probably have a setting that always forces commands
   forceCommand_ = false;

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

   // \todo add a check to see if mpc exists
   // \todo redirect std:error results into the console window too

   std::string const command("mpc " + arguments);

   Main::Console().Add("> " + command);

   FILE * const mpcOutput = popen(command.c_str(), "r");

   if (mpcOutput != NULL)
   {
      while (fgets(buffer, bufferSize - 1, mpcOutput) != NULL)
      {
         Main::Console().Add(buffer);
      }

      pclose(mpcOutput);
   }
   else
   {
      Error(ErrorNumber::ExternalProgramError, "Executing program mpc failed");
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

