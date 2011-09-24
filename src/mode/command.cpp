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
#include <pcrecpp.h>
#include <sstream>

#include "assert.hpp"
#include "settings.hpp"
#include "vimpc.hpp"
#include "window/console.hpp"
#include "window/error.hpp"
#include "window/songwindow.hpp"

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
   client_             (client),
   settings_           (settings)
{
   //! \TODO need to add specific finds somehow
   //        ie findsong, findalbum,  or something
   //        also need to provide automatically adding versions

   // \todo find a away to add aliases to tab completion
   commandTable_["!mpc"]      = &Command::Mpc;
   commandTable_["alias"]     = &Command::Alias;
   commandTable_["clear"]     = &Command::ClearScreen;
   commandTable_["connect"]   = &Command::Connect;
   commandTable_["consume"]   = &Command::Consume;
   commandTable_["echo"]      = &Command::Echo;
   commandTable_["find"]      = &Command::FindAny;
   commandTable_["findalbum"] = &Command::FindAlbum;
   commandTable_["findartist"]= &Command::FindArtist;
   commandTable_["findsong"]  = &Command::FindSong;
   commandTable_["move"]      = &Command::Move;
   commandTable_["pause"]     = &Command::Pause;
   commandTable_["play"]      = &Command::Play;
   commandTable_["q"]         = &Command::HideWindow;
   commandTable_["qall"]      = &Command::Quit;
   commandTable_["quit"]      = &Command::HideWindow;
   commandTable_["quitall"]   = &Command::Quit;
   commandTable_["random"]    = &Command::Random;
   commandTable_["redraw"]    = &Command::Redraw;
   commandTable_["repeat"]    = &Command::Repeat;
   commandTable_["set"]       = &Command::Set;
   commandTable_["single"]    = &Command::Single;
   commandTable_["shuffle"]   = &Command::Shuffle;
   commandTable_["swap"]      = &Command::Swap;
   commandTable_["stop"]      = &Command::Stop;
   commandTable_["volume"]    = &Command::Volume;

   commandTable_["tabfirst"]  = &Command::ChangeToWindow<First>;
   commandTable_["tablast"]   = &Command::ChangeToWindow<Last>;
   commandTable_["tabclose"]  = &Command::HideWindow;
   commandTable_["tabhide"]   = &Command::HideWindow;
   commandTable_["tabmove"]   = &Command::MoveWindow;
   commandTable_["tabrename"] = &Command::RenameWindow;

   commandTable_["rescan"]    = &Command::Rescan;
   commandTable_["update"]    = &Command::Update;

   commandTable_["next"]      = &Command::SkipSong<Player::Next>;
   commandTable_["previous"]  = &Command::SkipSong<Player::Previous>;

   commandTable_["browse"]    = &Command::SetActiveAndVisible<Ui::Screen::Browse>;
   commandTable_["console"]   = &Command::SetActiveAndVisible<Ui::Screen::Console>;
   commandTable_["help"]      = &Command::SetActiveAndVisible<Ui::Screen::Help>;
   commandTable_["library"]   = &Command::SetActiveAndVisible<Ui::Screen::Library>;
   commandTable_["playlist"]  = &Command::SetActiveAndVisible<Ui::Screen::Playlist>;
   commandTable_["lists"]     = &Command::SetActiveAndVisible<Ui::Screen::Lists>;

   //! \TODO add a command to export search results to a playlist
   commandTable_["load"]  = &Command::LoadPlaylist;
   commandTable_["save"]  = &Command::SavePlaylist;
   commandTable_["edit"]  = &Command::LoadPlaylist;
   commandTable_["write"] = &Command::SavePlaylist;
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
      pcrecpp::RE const blankCommand("^\\s*$");
      pcrecpp::RE const multipleCommandAlias("^(\\s*([^;]+)\\s*;?).*$");
      std::string       resolvedAlias(aliasTable_[command] + " " + arguments);

      std::string matchString;
      std::string commandString;

      while (multipleCommandAlias.FullMatch(resolvedAlias.c_str(), &matchString, &commandString) == true)
      {
         resolvedAlias = resolvedAlias.substr(matchString.size(), resolvedAlias.size());

         if (blankCommand.FullMatch(commandString) == false)
         {
            result = ExecuteCommand(commandString);
         }
      }
   }
   else
   {
      if (command == "connect" && settings_.SkipConfigConnects())
      {
         return true;
      }

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


bool Command::ClearScreen(std::string const & arguments)
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

bool Command::Pause(std::string const & arguments)
{
   return Player::Pause();
}

bool Command::Play(std::string const & arguments)
{
   return Player::Play(atoi(arguments.c_str()));
}

bool Command::Quit(std::string const & arguments)
{
   if ((forceCommand_ == true) || (settings_.StopOnQuit() == true))
   {
      Player::Stop();
   }

   return Player::Quit();
}

bool Command::Volume(std::string const & arguments)
{
   return Player::Volume(atoi(arguments.c_str()));
}

bool Command::LoadPlaylist(std::string const & arguments)
{
   return Player::LoadPlaylist(arguments);
}

bool Command::SavePlaylist(std::string const & arguments)
{
   return Player::SavePlaylist(arguments);
}

bool Command::Find(std::string const & arguments)
{
   //! \TODO if there is no results print an error rather than make an empty window
   SongWindow * window = screen_.CreateWindow(arguments);
   client_.ForEachSearchResult(window->Buffer(), static_cast<void (Mpc::Browse::*)(Mpc::Song *)>(&Mpc::Browse::Add));
   return true;
}

bool Command::FindAny(std::string const & arguments)
{
   client_.SearchAny(arguments);
   return Find("F:" + arguments);
}

bool Command::FindAlbum(std::string const & arguments)
{
   client_.SearchAlbum(arguments);
   return Find("FAL:" + arguments);
}

bool Command::FindArtist(std::string const & arguments)
{
   client_.SearchArtist(arguments);
   return Find("FAR:" + arguments);
}

bool Command::FindSong(std::string const & arguments)
{
   client_.SearchSong(arguments);
   return Find("FS:" + arguments);
}

bool Command::Random(std::string const & arguments)
{
   if (arguments.empty() == false)
   {
      bool const value = (arguments.compare("on") == 0);
      return Player::SetRandom(value);
   }

   return Player::ToggleRandom();
}

bool Command::Repeat(std::string const & arguments)
{
   if (arguments.empty() == false)
   {
      bool const value = (arguments.compare("on") == 0);
      return Player::SetRepeat(value);
   }

   return Player::ToggleRepeat();
}

bool Command::Single(std::string const & arguments)
{
   if (arguments.empty() == false)
   {
      bool const value = (arguments.compare("on") == 0);
      return Player::SetSingle(value);
   }

   return Player::ToggleSingle();
}

bool Command::Consume(std::string const & arguments)
{
   if (arguments.empty() == false)
   {
      bool const value = (arguments.compare("on") == 0);
      return Player::SetConsume(value);
   }

   return Player::ToggleConsume();
}

bool Command::Shuffle(std::string const & arguments)
{
   return Player::Shuffle();
}

bool Command::Move(std::string const & arguments)
{
   if ((arguments.find(" ") != string::npos))
   {
      std::string position1 = arguments.substr(0, arguments.find(" "));
      std::string position2 = arguments.substr(arguments.find(" ") + 1);
      client_.Move(atoi(position1.c_str()) - 1, atoi(position2.c_str()) - 1);
   }
   // \todo print error
   return true;
}

bool Command::Swap(std::string const & arguments)
{
   if ((arguments.find(" ") != string::npos))
   {
      std::string position1 = arguments.substr(0, arguments.find(" "));
      std::string position2 = arguments.substr(arguments.find(" ") + 1);
      client_.Swap(atoi(position1.c_str()) - 1, atoi(position2.c_str()) - 1);
   }
   // \todo print error
   return true;
}

bool Command::Redraw(std::string const & arguments)
{
   return Player::Redraw();
}

bool Command::Stop(std::string const & arguments)
{
   return Player::Stop();
}

bool Command::Rescan(std::string const & arguments)
{
   return Player::Rescan();
}

bool Command::Update(std::string const & arguments)
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
bool Command::SetActiveAndVisible(std::string const & arguments)
{
   screen_.SetActiveAndVisible((int32_t) MAINWINDOW);

   return true;
}

template <Command::Location LOCATION>
bool Command::ChangeToWindow(std::string const & arguments)
{
   uint32_t active = 0;

   if (LOCATION == Last)
   {
      active = screen_.VisibleWindows() - 1;
   }

   screen_.SetActiveWindow(active);

   return true;
}

bool Command::HideWindow(std::string const & arguments)
{
   if (arguments == "")
   {
      screen_.SetVisible(screen_.GetActiveWindow(), false);
   }
   else
   {
      int32_t window = screen_.GetWindowFromName(arguments);

      if (window != Ui::Screen::Unknown)
      {
         screen_.SetVisible(window, false);
      }
   }

   if (screen_.VisibleWindows() == 0)
   {
      return Quit("");
   }

   return true;
}

bool Command::MoveWindow(std::string const & arguments)
{
   screen_.MoveWindow(atoi(arguments.c_str()));
   return true;
}

bool Command::RenameWindow(std::string const & arguments)
{
   screen_.ActiveWindow().SetName(arguments);
   return true;
}


bool Command::ExecuteCommand(std::string command, std::string const & arguments)
{
   pcrecpp::RE const forceCheck("^.*!$");

   bool result     = true;
   forceCommand_   = false;

   if (forceCheck.FullMatch(command))
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
   char   port[8];

   // \todo add a check to see if mpc exists
   // \todo redirect std:error results into the console window too
   snprintf(port, 8, "%u", client_.Port());

   //
   std::string const command("MPD_HOST=" + client_.Hostname() + " MPD_PORT=" + std::string(port) + " mpc " + arguments);

   Main::Console().Add("> mpc " + arguments);

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

