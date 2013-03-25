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

#include <unistd.h>

#include <algorithm>
#include <pcrecpp.h>
#include <sstream>

#include "algorithm.hpp"
#include "assert.hpp"
#include "buffers.hpp"
#include "settings.hpp"
#include "vimpc.hpp"

#include "buffer/list.hpp"
#include "buffer/outputs.hpp"
#include "buffer/playlist.hpp"
#include "mode/normal.hpp"
#include "window/console.hpp"
#include "window/error.hpp"
#include "window/songwindow.hpp"

using namespace Ui;

// COMMANDS
Command::Command(Main::Vimpc * vimpc, Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings, Ui::Search & search, Ui::Normal & normalMode) :
   InputMode           (screen),
   Player              (screen, client, settings),
   initTabCompletion_  (true),
   forceCommand_       (false),
   queueCommands_      (false),
   aliasTable_         (),
   commandTable_       (),
   vimpc_              (vimpc),
   search_             (search),
   screen_             (screen),
   client_             (client),
   settings_           (settings),
   normalMode_         (normalMode)
{
   // \todo find a away to add aliases to tab completion
   AddCommand("!mpc",       &Command::Mpc,          true);
   AddCommand("add",        &Command::Add,          true);
   AddCommand("addall",     &Command::AddAll,       true);
   AddCommand("alias",      &Command::Alias,        false);
   AddCommand("clear",      &Command::ClearScreen,  false);
   AddCommand("connect",    &Command::Connect,      false);
   AddCommand("consume",    &Command::Consume,      true);
   AddCommand("crossfade",  &Command::Crossfade,    true);
   AddCommand("delete",     &Command::Delete,       true);
   AddCommand("deleteall",  &Command::DeleteAll,    true);
   AddCommand("disable",    &Command::Output<false>, true);
   AddCommand("disconnect", &Command::Disconnect,   true);
   AddCommand("echo",       &Command::Echo,         false);
   AddCommand("enable",     &Command::Output<true>, true);
   AddCommand("error",      &Command::EchoError,    false);
   AddCommand("find",       &Command::FindAny,      true);
   AddCommand("findalbum",  &Command::FindAlbum,    true);
   AddCommand("findartist", &Command::FindArtist,   true);
   AddCommand("findgenre",  &Command::FindGenre,    true);
   AddCommand("findsong",   &Command::FindSong,     true);
   AddCommand("move",       &Command::Move,         true);
   AddCommand("nohlsearch", &Command::NoHighlightSearch, false);
   AddCommand("normal",     &Command::Normal,       true);
   AddCommand("password",   &Command::Password,     true);
   AddCommand("pause",      &Command::Pause,        true);
   AddCommand("play",       &Command::Play,         true);
   AddCommand("q",          &Command::Quit,         false);
   AddCommand("qall",       &Command::QuitAll,      false);
   AddCommand("quit",       &Command::Quit,         false);
   AddCommand("quitall",    &Command::QuitAll,      false);
   AddCommand("random",     &Command::Random,       true);
   AddCommand("reconnect",  &Command::Reconnect,    false);
   AddCommand("redraw",     &Command::Redraw,       false);
   AddCommand("repeat",     &Command::Repeat,       true);
   AddCommand("set",        &Command::Set,          false);
   AddCommand("seek",       &Command::SeekTo,       true);
   AddCommand("seek+",      &Command::Seek<1>,      true);
   AddCommand("seek-",      &Command::Seek<-1>,     true);
   AddCommand("single",     &Command::Single,       true);
   AddCommand("shuffle",    &Command::Shuffle,      true);
   AddCommand("sleep",      &Command::Sleep,        false);
   AddCommand("swap",       &Command::Swap,         true);
   AddCommand("stop",       &Command::Stop,         true);
   AddCommand("toggle",     &Command::ToggleOutput, true);
   AddCommand("volume",     &Command::Volume,       true);

   AddCommand("map",        &Command::Map,          false);
   AddCommand("unmap",      &Command::Unmap,        false);
   AddCommand("wmap",       &Command::WindowMap,    false);
   AddCommand("wunmap",     &Command::WindowUnmap,  false);
   AddCommand("tmap",       &Command::TabMap,       false);
   AddCommand("tunmap",     &Command::TabUnmap,     false);
   AddCommand("tabmap",     &Command::TabMap,       false);
   AddCommand("tabunmap",   &Command::TabUnmap,     false);

   AddCommand("tabfirst",   &Command::ChangeToWindow<First>, false);
   AddCommand("tablast",    &Command::ChangeToWindow<Last>,  false);
   AddCommand("tabclose",   &Command::HideWindow,            false);
   AddCommand("tabhide",    &Command::HideWindow,            false);
   AddCommand("tabmove",    &Command::MoveWindow,            false);
   AddCommand("tabrename",  &Command::RenameWindow,          false);

   AddCommand("rescan",     &Command::Rescan, true);
   AddCommand("update",     &Command::Update, true);

   AddCommand("next",       &Command::SkipSong<Player::Next>,     true);
   AddCommand("previous",   &Command::SkipSong<Player::Previous>, true);

   AddCommand("browse",     &Command::SetActiveAndVisible<Ui::Screen::Browse>,    false);
   AddCommand("console",    &Command::SetActiveAndVisible<Ui::Screen::Console>,   false);
   AddCommand("help",       &Command::SetActiveAndVisible<Ui::Screen::Help>,      true);
   AddCommand("library",    &Command::SetActiveAndVisible<Ui::Screen::Library>,   true);
   AddCommand("directory",  &Command::SetActiveAndVisible<Ui::Screen::Directory>, true);
   AddCommand("playlist",   &Command::SetActiveAndVisible<Ui::Screen::Playlist>,  true);
   AddCommand("outputs",    &Command::SetActiveAndVisible<Ui::Screen::Outputs>,   true);
   AddCommand("lists",      &Command::SetActiveAndVisible<Ui::Screen::Lists>,     true);

   AddCommand("load",       &Command::LoadPlaylist, true);
   AddCommand("save",       &Command::SavePlaylist, true);
   AddCommand("edit",       &Command::LoadPlaylist, true);
   AddCommand("write",      &Command::SavePlaylist, true);
   AddCommand("toplaylist", &Command::ToPlaylist,   true);

#ifdef __DEBUG_PRINTS
   AddCommand("debug",         &Command::SetActiveAndVisible<Ui::Screen::DebugConsole>,    false);
   AddCommand("debug-getmeta", &Command::DebugClient<&Mpc::Client::GetAllMetaInformation>, true);
   AddCommand("debug-idle",    &Command::DebugClient<&Mpc::Client::IdleMode>,              true);
   AddCommand("test-screen",   &Command::TestScreen,                                       false);
#endif

   // Add all settings to command table to provide tab completion
   std::vector<std::string> const AllSettings = settings_.AvailableSettings();

   for (uint32_t i = 0; i < AllSettings.size(); ++i)
   {
      settingsTable_.push_back("set " + AllSettings.at(i));
   }
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

   InputMode::GenerateInputString(input);
}


void Command::AddCommand(std::string const & name, CommandFunction command, bool requiresConnection)
{
   commandTable_[name]       = command;
   requiresConnection_[name] = requiresConnection;
}

bool Command::ExecuteCommand(std::string const & input)
{
   pcrecpp::RE const blankCommand("^\\s*$");
   pcrecpp::RE const multipleCommands("^(\\s*([^;]+)\\s*;\\s*).*$");

   std::string matchString, commandString;
   std::string command, arguments;

   SplitCommand(input, command, arguments);

   std::string fullCommand(command + " " + arguments);

   if (aliasTable_.find(command) != aliasTable_.end())
   {
      fullCommand = aliasTable_[command] + " " + arguments;
   }

   if ((multipleCommands.FullMatch(fullCommand.c_str(), &matchString, &commandString) == true) &&
       (command != "alias")) // In the case of alias we don't actually want to run the commands
   {
      // There are multiple commands seperated gy ';' we have to handle each one
      while (multipleCommands.FullMatch(fullCommand.c_str(), &matchString, &commandString) == true)
      {
         fullCommand = fullCommand.substr(matchString.size(), fullCommand.size());

         if (blankCommand.FullMatch(commandString) == false)
         {
            ExecuteCommand(commandString);
         }
      }

      // The last command does not need a ';'
      if (blankCommand.FullMatch(fullCommand) == false)
      {
         ExecuteCommand(fullCommand);
      }
   }
   else if (aliasTable_.find(command) != aliasTable_.end())
   {
      // We are a single command alias, but this could actually be
      // another alias, so ensure to look this up in the table to
      // by calling this fucking again
      ExecuteCommand(fullCommand);
   }
   else if ((arguments == "") && (Algorithm::isNumeric(command) == true))
   {
      int32_t line = atoi(command.c_str());

      if (line >= 1)
      {
         --line;
      }

      screen_.ScrollTo(line);
   }
   else
   {
      // Ignore the connect command when starting up if -h/-p used
      // on the command line
      if (command == "connect" && settings_.SkipConfigConnects())
      {
         return true;
      }

      // Just a normal command
      ExecuteCommand(command, arguments);
   }

   return true;
}

void Command::SetQueueCommands(bool enabled)
{
   queueCommands_ = enabled;
}


void Command::ExecuteQueuedCommands()
{
   for (CommandQueue::const_iterator it = commandQueue_.begin(); it != commandQueue_.end(); ++it)
   {
      ExecuteCommand((*it).first, (*it).second);
   }

   commandQueue_.clear();
}

bool Command::RequiresConnection(std::string const & command)
{
   return requiresConnection_[command];

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


void Command::ClearScreen(std::string const & arguments)
{
   Player::ClearScreen();
}

void Command::Pause(std::string const & arguments)
{
   Player::Pause();
}

void Command::Play(std::string const & arguments)
{
   std::vector<std::string> args = SplitArguments(arguments);

   if (args.size() == 0)
   {
      if (screen_.GetActiveWindow() == Ui::Screen::Playlist)
      {
         int32_t line = screen_.Window(Ui::Screen::Playlist).CurrentLine();

         if (line >= 0)
         {
            Player::Play(line);
         }
      }
      else
      {
         Player::Play(0);
      }
   }
   else if (args.size() == 1)
   {
      int32_t const SongId = atoi(args[0].c_str()) - 1;
      Player::Play(SongId);
   }
   else
   {
      ErrorString(ErrorNumber::InvalidParameter, "too many parameters");
   }
}

void Command::Add(std::string const & arguments)
{
   if (client_.Connected() == true)
   {
      screen_.Initialise(Ui::Screen::Playlist);
      client_.Add(arguments);
      client_.AddComplete();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Command::AddAll(std::string const & arguments)
{
   if (client_.Connected() == true)
   {
      screen_.Initialise(Ui::Screen::Playlist);
      client_.AddAllSongs();
      client_.AddComplete();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Command::Delete(std::string const & arguments)
{
   if (client_.Connected() == true)
   {
      screen_.Initialise(Ui::Screen::Playlist);

      std::vector<std::string> args = SplitArguments(arguments);

      if (args.size() == 2)
      {
         uint32_t pos1 = atoi(args[0].c_str()) - 1;
         uint32_t pos2 = atoi(args[1].c_str()) - 1;

         client_.Delete(pos1, pos2 + 1);
         Main::Playlist().Remove(((pos1 < pos2) ? pos1 : pos2), ((pos1 < pos2) ? pos2 - pos1 : pos1 - pos2) + 1);
      }
      else if (args.size() == 1)
      {
         client_.Delete(atoi(args[0].c_str()) - 1);
         Main::Playlist().Remove(atoi(args[0].c_str()) - 1, 1);
      }
      else
      {
         //\TODO delete selected song
      }
   }
}

void Command::DeleteAll(std::string const & arguments)
{
   client_.Clear();
   Main::Playlist().Clear();
}

template <int Delta>
void Command::Seek(std::string const & arguments)
{
   uint32_t time = 0;
   size_t pos    = arguments.find_first_of(":");

   if (pos != std::string::npos)
   {
      std::string minutes = arguments.substr(0, pos);
      std::string seconds = arguments.substr(pos + 1);
      time = (atoi(minutes.c_str()) * 60) + atoi(seconds.c_str());
   }
   else
   {
      time = atoi(arguments.c_str());
   }

   Player::Seek(Delta * time);
}

void Command::SeekTo(std::string const & arguments)
{
   uint32_t time = 0;
   size_t pos    = arguments.find_first_of(":");

   if (pos != std::string::npos)
   {
      std::string minutes = arguments.substr(0, pos);
      std::string seconds = arguments.substr(pos + 1);
      time = (atoi(minutes.c_str()) * 60) + atoi(seconds.c_str());
   }
   else
   {
      time = atoi(arguments.c_str());
   }

   Player::SeekTo(time);
}

void Command::Quit(std::string const & arguments)
{
   if (settings_.Get(Setting::SingleQuit) == true)
   {
      QuitAll(arguments);
   }
   else
   {
      HideWindow(arguments);
   }
}

void Command::QuitAll(std::string const & arguments)
{
   if ((forceCommand_ == true) || (settings_.Get(Setting::StopOnQuit) == true))
   {
      Player::Stop();
   }

   Player::Quit();
}

void Command::Volume(std::string const & arguments)
{
   uint32_t Vol = atoi(arguments.c_str());

   if ((Vol <= 100) && (Vol >= 0))
   {
      Player::Volume(Vol);
   }
   else
   {
      ErrorString(ErrorNumber::InvalidParameter);
   }
}


void Command::Connect(std::string const & arguments)
{
   size_t   pos  = arguments.find_first_of(" ");
   uint32_t port = 0;

   std::string hostname = arguments.substr(0, pos);

   if (pos != std::string::npos)
   {
      port = atoi(arguments.substr(pos + 1).c_str());
   }

   client_.Connect(hostname, port);
}

void Command::Disconnect(std::string const & arguments)
{
   client_.Disconnect();
}

void Command::Reconnect(std::string const & arguments)
{
   client_.Reconnect();
}

void Command::Password(std::string const & password)
{
   client_.Password(password.c_str());
}

void Command::Echo(std::string const & echo)
{
   Main::Console().Add(echo);
}

void Command::EchoError(std::string const & arguments)
{
   int32_t error = atoi(arguments.substr(0, arguments.find(" ")).c_str());
   ErrorString(error, arguments.substr(arguments.find(" ") + 1));
}

void Command::Sleep(std::string const & seconds)
{
   vimpc_->ChangeMode('\n', "");
   usleep(1000 * 1000 * atoi(seconds.c_str()));
}


template <bool ON>
void Command::Output(std::string const & arguments)
{
   int32_t output = -1;

   screen_.Initialise(Ui::Screen::Outputs);

   if (arguments == "")
   {
      output = screen_.Window(Ui::Screen::Outputs).CurrentLine();
   }
   else if (Algorithm::isNumeric(arguments.c_str()) == true)
   {
      output = atoi(arguments.c_str());
   }
   else
   {
      for(unsigned int i = 0; i < Main::Outputs().Size(); ++i)
      {
         if (Algorithm::iequals(Main::Outputs().Get(i)->Name(), arguments) == true)
         {
            output = i;
            break;
         }
      }
   }

   if ((output < static_cast<int32_t>(Main::Outputs().Size())) && (output >= 0))
   {
      if (ON == true)
      {
         client_.EnableOutput(Main::Outputs().Get(output));
         Main::Outputs().Get(output)->SetEnabled(true);
      }
      else
      {
         client_.DisableOutput(Main::Outputs().Get(output));
         Main::Outputs().Get(output)->SetEnabled(false);
      }
   }
   else
   {
      ErrorString(ErrorNumber::NoOutput);
   }
}

void Command::ToggleOutput(std::string const & arguments)
{
   int32_t output = -1;

   screen_.Initialise(Ui::Screen::Outputs);

   if (arguments == "")
   {
      output = screen_.GetSelected(Ui::Screen::Outputs);
   }
   else if (Algorithm::isNumeric(arguments.c_str()) == true)
   {
      output = atoi(arguments.c_str());
   }
   else
   {
      output = Player::FindOutput(arguments);
   }

   if ((output < static_cast<int32_t>(Main::Outputs().Size())) && (output >= 0))
   {
      Player::ToggleOutput(output);
   }
   else
   {
      ErrorString(ErrorNumber::NoOutput);
   }
}

void Command::LoadPlaylist(std::string const & arguments)
{
   if (arguments != "")
   {
      client_.LoadPlaylist(arguments);
   }
   else
   {
      ErrorString(ErrorNumber::NoParameter);
   }
}

void Command::SavePlaylist(std::string const & arguments)
{
   if (arguments != "")
   {
      if (Main::Lists().Index(Mpc::List(arguments)) == -1)
      {
         Main::Lists().Add(arguments);
         Main::Lists().Sort();
      }
      else
      {
         client_.RemovePlaylist(arguments);
      }

      client_.SavePlaylist(arguments);
   }
   else
   {
      ErrorString(ErrorNumber::NoParameter);
   }
}

void Command::ToPlaylist(std::string const & arguments)
{
   if (arguments != "")
   {
      screen_.ActiveWindow().Save(arguments);
   }
   else
   {
      ErrorString(ErrorNumber::NoParameter);
   }
}

void Command::Find(std::string const & arguments)
{
   if (forceCommand_ == true)
   {
      Main::PlaylistTmp().Clear();
      client_.ForEachSearchResult(Main::PlaylistTmp(), static_cast<void (Mpc::Playlist::*)(Mpc::Song *)>(&Mpc::Playlist::Add));

      if (Main::PlaylistTmp().Size() == 0)
      {
         ErrorString(ErrorNumber::FindNoResults);
      }
      else
      {
         Mpc::CommandList list(client_);

         for (uint32_t i = 0; i < Main::PlaylistTmp().Size(); ++i)
         {
            client_.Add(Main::PlaylistTmp().Get(i));
            Main::Playlist().Add(Main::PlaylistTmp().Get(i));
         }
      }
   }
   else
   {
      SongWindow * window = screen_.CreateSongWindow(arguments);
      client_.ForEachSearchResult(window->Buffer(), static_cast<void (Main::Buffer<Mpc::Song *>::*)(Mpc::Song *)>(&Mpc::Browse::Add));

      if (window->BufferSize() > 0)
      {
         screen_.SetActiveAndVisible(screen_.GetWindowFromName(window->Name()));
      }
      else
      {
         screen_.SetVisible(screen_.GetWindowFromName(window->Name()), false);
         ErrorString(ErrorNumber::FindNoResults);
      }
   }
}

void Command::FindAny(std::string const & arguments)
{
   client_.SearchAny(arguments);
   Find("F:" + arguments);
}

void Command::FindAlbum(std::string const & arguments)
{
   client_.SearchAlbum(arguments);
   Find("F:" + arguments);
}

void Command::FindArtist(std::string const & arguments)
{
   client_.SearchArtist(arguments);
   Find("F:" + arguments);
}

void Command::FindGenre(std::string const & arguments)
{
   client_.SearchGenre(arguments);
   Find("F:" + arguments);
}

void Command::FindSong(std::string const & arguments)
{
   client_.SearchSong(arguments);
   Find("F:" + arguments);
}

void Command::Map(std::string const & arguments)
{
   if ((arguments.find(" ") != string::npos))
   {
      std::string key     = arguments.substr(0, arguments.find(" "));
      std::string mapping = arguments.substr(arguments.find(" ") + 1);

      normalMode_.Map(key, mapping);
   }
   else if (arguments == "")
   {
      Ui::Normal::MapNameTable mappings = normalMode_.Mappings();

      if (mappings.size() > 0)
      {
         Ui::Normal::MapNameTable::const_iterator it = mappings.begin();

         PagerWindow * pager = screen_.GetPagerWindow();
         pager->Clear();

         for (; it != mappings.end(); ++it)
         {
            pager->AddLine(it->first + "   " + it->second);
         }

         screen_.ShowPagerWindow();
      }
      else
      {
         ErrorString(ErrorNumber::NoSuchMapping);
      }
   }
}

void Command::Unmap(std::string const & arguments)
{
   normalMode_.Unmap(arguments);
}

void Command::TabMap(std::string const & arguments)
{
   std::string tabname, args;
   SplitCommand(arguments, tabname, args);

   TabMap(tabname, args);
}

void Command::TabMap(std::string const & tabname, std::string const & arguments)
{
   std::vector<std::string> args = SplitArguments(arguments);

   if (args.size() == 2)
   {
      std::string const key     = args[0];
      std::string const mapping = args[1];
      normalMode_.WindowMap(screen_.GetWindowFromName(tabname), key, mapping);
   }
   else if (args.size() == 0)
   {
      Ui::Normal::MapNameTable mappings;
      
      if (args.size() == 0)
      {
         mappings = normalMode_.WindowMappings(screen_.GetWindowFromName(tabname));
      }

      if (mappings.size() > 0)
      {
         Ui::Normal::MapNameTable::const_iterator it = mappings.begin();

         PagerWindow * pager = screen_.GetPagerWindow();
         pager->Clear();

         for (; it != mappings.end(); ++it)
         {
            pager->AddLine(it->first + "   " + it->second);
         }

         screen_.ShowPagerWindow();
      }
      else
      {
         ErrorString(ErrorNumber::NoSuchMapping);
      }
   }
   else
   {
      Error(ErrorNumber::InvalidParameter, "Unexpected Arguments");
   }
}


void Command::TabUnmap(std::string const & arguments)
{
   std::vector<std::string> args = SplitArguments(arguments);

   if (args.size() == 2)
   {
      std::string const window = args[0];
      std::string const key    = args[1];
      normalMode_.WindowUnmap(screen_.GetWindowFromName(window), key);
   }
}

void Command::WindowMap(std::string const & arguments)
{
   TabMap(screen_.GetNameFromWindow(screen_.GetActiveWindow()), arguments);
}

void Command::WindowUnmap(std::string const & arguments)
{
   normalMode_.WindowUnmap(screen_.GetActiveWindow(), arguments);
}

void Command::Random(std::string const & arguments)
{
   if (arguments.empty() == false)
   {
      bool const value = (arguments.compare("on") == 0);
      Player::SetRandom(value);
   }
   else
   {
      Player::ToggleRandom();
   }
}

void Command::Repeat(std::string const & arguments)
{
   if (arguments.empty() == false)
   {
      bool const value = (arguments.compare("on") == 0);
      Player::SetRepeat(value);
   }
   else
   {
      Player::ToggleRepeat();
   }
}

void Command::Single(std::string const & arguments)
{
   if (arguments.empty() == false)
   {
      bool const value = (arguments.compare("on") == 0);
      Player::SetSingle(value);
   }
   else
   {
      Player::ToggleSingle();
   }
}

void Command::Consume(std::string const & arguments)
{
   if (arguments.empty() == false)
   {
      bool const value = (arguments.compare("on") == 0);
      Player::SetConsume(value);
   }
   else
   {
      Player::ToggleConsume();
   }
}

void Command::Crossfade(std::string const & arguments)
{
   if (arguments.empty() == false)
   {
      Player::SetCrossfade(static_cast<uint32_t>(atoi(arguments.c_str())));
   }
   else
   {
      Player::ToggleCrossfade();
   }
}

void Command::Shuffle(std::string const & arguments)
{
   Player::Shuffle();
}

void Command::Move(std::string const & arguments)
{
   screen_.Initialise(Ui::Screen::Playlist);

   if ((arguments.find(" ") != string::npos))
   {
      int32_t position1 = atoi(arguments.substr(0, arguments.find(" ")).c_str());
      int32_t position2 = atoi(arguments.substr(arguments.find(" ") + 1).c_str());

      if (position1 >= screen_.ActiveWindow().BufferSize() - 1)
      {
         position1 = Main::Playlist().Size();
      }
      else if (position1 <= 1)
      {
         position1 = 1;
      }

      if (position2 >= screen_.ActiveWindow().BufferSize() - 1)
      {
         position2 = Main::Playlist().Size();
      }
      else if (position2 <= 1)
      {
         position2 = 1;
      }

      client_.Move(position1 - 1, position2 - 1);

      Mpc::Song * song = Main::Playlist().Get(position1 - 1);
      Main::Playlist().Remove(position1 - 1, 1);
      Main::Playlist().Add(song, position2 - 1);
      screen_.Update();
   }
   else
   {
      Error(ErrorNumber::InvalidParameter, "Expected two arguments");
   }
}

void Command::NoHighlightSearch(std::string const & arguments)
{
   search_.SetHighlightSearch(false);
}

void Command::Normal(std::string const & arguments)
{
   vimpc_->ChangeMode('\n', "");
   normalMode_.Execute(arguments);
}

void Command::Swap(std::string const & arguments)
{
   screen_.Initialise(Ui::Screen::Playlist);

   if ((arguments.find(" ") != string::npos))
   {
      std::string position1 = arguments.substr(0, arguments.find(" "));
      std::string position2 = arguments.substr(arguments.find(" ") + 1);
      client_.Swap(atoi(position1.c_str()) - 1, atoi(position2.c_str()) - 1);
   }
   else
   {
      Error(ErrorNumber::InvalidParameter, "Expected two arguments");
   }
}

void Command::Redraw(std::string const & arguments)
{
   Player::Redraw();
}

void Command::Stop(std::string const & arguments)
{
   Player::Stop();
}

void Command::Rescan(std::string const & arguments)
{
   client_.Rescan(arguments);
}

void Command::Update(std::string const & arguments)
{
   client_.Update(arguments);
}


//Implementation of skipping functions
template <Ui::Player::Skip SKIP>
void Command::SkipSong(std::string const & arguments)
{
   uint32_t count = atoi(arguments.c_str());
   count = (count == 0) ? 1 : count;
   Player::SkipSong(SKIP, count);
}


//Implementation of window change function
template <Ui::Screen::MainWindow MAINWINDOW>
void Command::SetActiveAndVisible(std::string const & arguments)
{
   screen_.SetActiveAndVisible(static_cast<int32_t>(MAINWINDOW));
}

template <Command::Location LOCATION>
void Command::ChangeToWindow(std::string const & arguments)
{
   uint32_t active = 0;

   if (LOCATION == Last)
   {
      active = screen_.VisibleWindows() - 1;
   }

   screen_.SetActiveWindow(active);
}

void Command::HideWindow(std::string const & arguments)
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
      else
      {
         ErrorString(ErrorNumber::TabDoesNotExist, arguments);
      }
   }

   if (screen_.VisibleWindows() == 0)
   {
      QuitAll("");
   }
}

void Command::MoveWindow(std::string const & arguments)
{
   screen_.MoveWindow(atoi(arguments.c_str()));
}

void Command::RenameWindow(std::string const & arguments)
{
   if ((arguments.find(" ") != string::npos))
   {
      std::string oldname = arguments.substr(0, arguments.find(" "));
      std::string newname = arguments.substr(arguments.find(" ") + 1);

      int32_t id = screen_.GetWindowFromName(oldname);

      if (id != Ui::Screen::Unknown)
      {
         screen_.Window(id).SetName(newname);
      }
      else
      {
         ErrorString(ErrorNumber::TabDoesNotExist, oldname);
      }
   }
   else
   {
      screen_.ActiveWindow().SetName(arguments);
   }
}

template <Ui::Command::ClientFunction FUNCTION>
void Command::DebugClient(std::string const & arguments)
{
   Ui::Command::ClientFunction func = FUNCTION;
   (client_.*func)();
}

void Command::TestScreen(std::string const & arguments)
{
   screen_.ActiveWindow().ScrollTo(999999);
   screen_.ScrollTo(screen_.ActiveWindow().Playlist(0));
   screen_.ScrollTo(screen_.ActiveWindow().Current());
   screen_.ActiveWindow().ScrollTo(0);
   screen_.Invalidate(screen_.GetActiveWindow());
}


bool Command::ExecuteCommand(std::string command, std::string const & arguments)
{
   pcrecpp::RE const forceCheck("^.*!$");

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
      if ((RequiresConnection(commandToExecute) == false) || (queueCommands_ == false) || (client_.Connected() == true))
      {
         CommandTable::const_iterator const it = commandTable_.find(commandToExecute);
         CommandFunction const commandFunction = it->second;
   
         (*this.*commandFunction)(arguments);
      }
      else if ((RequiresConnection(commandToExecute) == true) && ((queueCommands_ == true) && (client_.Connected() == false)))
      {
         CommandArgPair pair(commandToExecute, arguments);
         commandQueue_.push_back(pair);
      }
   }
   else if (validCommandCount > 1)
   {
      ErrorString(ErrorNumber::CommandAmbiguous, command);
   }
   else
   {
      ErrorString(ErrorNumber::CommandNonexistant, command);
   }

   forceCommand_ = false;

   return true;
}

void Command::SplitCommand(std::string const & input, std::string & command, std::string & arguments)
{
   std::stringstream commandStream(input);
   std::getline(commandStream, command,   ' ');
   std::getline(commandStream, arguments, '\n');
}

std::vector<std::string> Command::SplitArguments(std::string const & input, char delimeter)
{
   std::vector<std::string> arguments;
   std::string argument;

   std::stringstream stream(input);

   while (stream.eof() == false)
   {
      std::getline(stream, argument, delimeter);

      if (argument != "")
      {
         arguments.push_back(argument);
      }
   }

   return arguments;
}

void Command::Set(std::string const & arguments)
{
   if (arguments != "")
   {
      settings_.Set(arguments);
   }
   else
   {
      std::vector<std::string> settings  = settings_.AvailableSettings();
      std::vector<std::string>::iterator it = settings.begin();

      PagerWindow * pager = screen_.GetPagerWindow();
      pager->Clear();

      for (; it != settings.end(); ++it)
      {
         if ((((*it).size() < 2) || ((*it).substr(0, 2) != "no")) && 
             (settings_.GetBool(*it) == true))
         {
            pager->AddLine(*it);
         }
         else if (settings_.GetString(*it) != "")
         {
            std::string const value = settings_.GetString(*it);
            pager->AddLine(*it + "=" + value);
         }
      }

      screen_.ShowPagerWindow();
   }
}

void Command::Mpc(std::string const & arguments)
{
   static uint32_t const bufferSize = 512;
   char   buffer[bufferSize];
   char   port[8];

   // \todo redirect stderr results into the console window too
   snprintf(port, 8, "%u", client_.Port());

   // Ensure that we use the same mpd_host and port for mpc that
   // we are using but still allow the person running the command
   // to do -h and -p flags
   std::string const command("MPD_HOST=" + client_.Hostname() + " MPD_PORT=" + std::string(port) +
                             " mpc " + arguments + " 2>&1");

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
      ErrorString(ErrorNumber::ExternalProgramError, "mpc");
   }
}

void Command::Alias(std::string const & input)
{
   std::string command, arguments;

   SplitCommand(input, command, arguments);

   aliasTable_[command] = arguments;
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

   if (initTabCompletion_ == true)
   {
      tabStart = command;
      loadTable_.clear();

      screen_.Initialise(Ui::Screen::Lists);

      for (uint32_t i = 0; i < Main::Lists().Size(); ++i)
      {
         loadTable_.push_back("load " + Main::Lists().Get(i).name_);
         loadTable_.push_back("edit " + Main::Lists().Get(i).name_);
      }
   }

   if (tabStart.find("set ") == 0)
   {
      return TabComplete(tabStart, settingsTable_, TabCompletionMatch<std::string>(tabStart));
   }
   else if ((tabStart.find("load ") == 0) || (tabStart.find("edit ") == 0))
   {
      return TabComplete(tabStart, loadTable_,  TabCompletionMatch<std::string>(tabStart));
   }
   else
   {
      return TabComplete(tabStart, commandTable_, TabCompletionMatch<CommandFunction>(tabStart));
   }
}


template <typename T, typename U>
std::string Command::TabComplete(std::string const & tabStart, T const & table, TabCompletionMatch<U> const & completor)
{
   static typename T::const_iterator tabIterator(table.begin());

   std::string result;

   if (initTabCompletion_ == true)
   {
      initTabCompletion_ = false;
      tabIterator        = table.begin();
   }

   tabIterator = find_if(tabIterator, table.end(), completor);

   if (tabIterator != table.end())
   {
      result = TabGetCompletion(tabIterator);
      ++tabIterator;
   }
   else
   {
      initTabCompletion_ = true;
      result             = tabStart;
   }

   return result;
}

std::string Command::TabGetCompletion(CommandTable::const_iterator it)
{
   return it->first;
}

std::string Command::TabGetCompletion(TabCompTable::const_iterator it)
{
   return *it;
}

/* vim: set sw=3 ts=3: */
