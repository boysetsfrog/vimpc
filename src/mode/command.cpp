/*
   Vimpc
   Copyright (C) 2013 Nathan Sweetman

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
#include <sstream>

#ifdef HAVE_TAGLIB_H
#include <taglib/tag.h>
#include <taglib/taglib.h>
#include <taglib/fileref.h>
#endif

#ifdef HAVE_TEST_H
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TextOutputter.h>
#endif

#include "algorithm.hpp"
#include "assert.hpp"
#include "buffers.hpp"
#include "regex.hpp"
#include "settings.hpp"
#include "tag.hpp"
#include "vimpc.hpp"

#include "buffer/directory.hpp"
#include "buffer/list.hpp"
#include "buffer/outputs.hpp"
#include "buffer/playlist.hpp"
#include "mode/normal.hpp"
#include "window/console.hpp"
#include "window/debug.hpp"
#include "window/error.hpp"
#include "window/songwindow.hpp"

using namespace Ui;

static std::list<std::string>             Queue;

static Atomic(bool)                       Running(true);
static Mutex                              QueueMutex;
static ConditionVariable                  Condition;


// COMMANDS
Command::Command(Main::Vimpc * vimpc, Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Main::Settings & settings, Ui::Search & search, Ui::Normal & normalMode) :
   InputMode           (screen),
   Player              (screen, client, clientState, settings),
   initTabCompletion_  (true),
   forceCommand_       (false),
   queueCommands_      (false),
   connectAttempt_     (false),
   count_              (0),
   line_               (-1),
   currentLine_        (-1),
   aliasTable_         (),
   commandTable_       (),
   vimpc_              (vimpc),
   search_             (search),
   screen_             (screen),
   client_             (client),
   clientState_        (clientState),
   settings_           (settings),
   normalMode_         (normalMode)
#ifdef HAVE_TEST_H
   ,testThread_        (Thread(&Command::TestExecutor, this))
#endif
{
   // \todo find a away to add aliases to tab completion
   // Command, RequiresConnection, SupportsRange, Function
   AddCommand("!mpc",       true,  false, &Command::Mpc);
   AddCommand("a",          true,  true,  &Command::Add);
   AddCommand("add",        true,  true,  &Command::Add);
   AddCommand("addall",     true,  false, &Command::AddAll);
   AddCommand("alias",      false, false, &Command::Alias);
   AddCommand("clear",      false, false, &Command::ClearScreen);
   AddCommand("connect",    false, false, &Command::Connect);
   AddCommand("consume",    true,  false, &Command::Consume);
   AddCommand("crossfade",  true,  false, &Command::Crossfade);
   AddCommand("d",          true,  true,  &Command::Delete);
   AddCommand("delete",     true,  true,  &Command::Delete);
   AddCommand("deleteall",  true,  false, &Command::DeleteAll);
   AddCommand("disable",    true,  true,  &Command::Output<false>);
   AddCommand("disconnect", true,  false, &Command::Disconnect);
   AddCommand("echo",       false, false, &Command::Echo);
   AddCommand("enable",     true,  true,  &Command::Output<true>);
   AddCommand("error",      false, false, &Command::EchoError);
   AddCommand("find",       true,  false, &Command::FindAny);
   AddCommand("findalbum",  true,  false, &Command::FindAlbum);
   AddCommand("findartist", true,  false, &Command::FindArtist);
   AddCommand("findgenre",  true,  false, &Command::FindGenre);
   AddCommand("findsong",   true,  false, &Command::FindSong);
   AddCommand("move",       true,  true,  &Command::Move);
   AddCommand("mute",       true,  false, &Command::Mute);
   AddCommand("nohlsearch", false, false, &Command::NoHighlightSearch);
   AddCommand("normal",     true,  false, &Command::Normal);
   AddCommand("password",   true,  false, &Command::Password);
   AddCommand("pause",      true,  false, &Command::Pause);
   AddCommand("play",       true,  false,  &Command::Play);
   AddCommand("q",          false, false, &Command::Quit);
   AddCommand("qall",       false, false, &Command::QuitAll);
   AddCommand("quit",       false, false, &Command::Quit);
   AddCommand("quitall",    false, false, &Command::QuitAll);
   AddCommand("random",     true,  false, &Command::Random);
   AddCommand("reconnect",  false, false, &Command::Reconnect);
   AddCommand("redraw",     false, false, &Command::Redraw);
   AddCommand("repeat",     true,  false, &Command::Repeat);
   AddCommand("set",        false, false, &Command::Set);
   AddCommand("seek",       true,  false, &Command::SeekTo);
   AddCommand("seek+",      true,  false, &Command::Seek<1>);
   AddCommand("seek-",      true,  false, &Command::Seek<-1>);
   AddCommand("single",     true,  false, &Command::Single);
   AddCommand("shuffle",    true,  false, &Command::Shuffle);
   AddCommand("sleep",      false, false, &Command::Sleep);
#ifdef TAG_SUPPORT
   AddCommand("substitute", false, true,  &Command::Substitute);
   AddCommand("s",          false, true,  &Command::Substitute);
#endif
   AddCommand("swap",       true,  false,  &Command::Swap);
   AddCommand("stop",       true,  false, &Command::Stop);
   AddCommand("toggle",     true,  true,  &Command::ToggleOutput);
   AddCommand("unalias",    false, false, &Command::Unalias);
   AddCommand("volume",     true,  false,  &Command::Volume);

   AddCommand("map",        false, false, &Command::Map);
   AddCommand("unmap",      false, false, &Command::Unmap);
   AddCommand("wmap",       false, false, &Command::WindowMap);
   AddCommand("wunmap",     false, false, &Command::WindowUnmap);
   AddCommand("tmap",       false, false, &Command::TabMap);
   AddCommand("tunmap",     false, false, &Command::TabUnmap);
   AddCommand("tabmap",     false, false, &Command::TabMap);
   AddCommand("tabunmap",   false, false, &Command::TabUnmap);

   AddCommand("tabfirst",   false, false, &Command::ChangeToWindow<First>);
   AddCommand("tablast",    false, false, &Command::ChangeToWindow<Last>);
   AddCommand("tabnext",    false, false, &Command::ChangeToWindow<Next>);
   AddCommand("tabprevious",false, false, &Command::ChangeToWindow<Previous>);
   AddCommand("tabclose",   false, false, &Command::HideWindow);
   AddCommand("tabhide",    false, false, &Command::HideWindow);
   AddCommand("tabmove",    false, false, &Command::MoveWindow);
   AddCommand("tabrename",  false, false, &Command::RenameWindow);

   AddCommand("highlight",  false, false, &Command::SetColour);

   AddCommand("rescan",     true,  false, &Command::Rescan);
   AddCommand("update",     true,  false, &Command::Update);

   AddCommand("next",       true,  false, &Command::SkipSong<Player::Next>);
   AddCommand("previous",   true,  false, &Command::SkipSong<Player::Previous>);

   AddCommand("browse",      false, false, &Command::SetActiveAndVisible<Ui::Screen::Browse>);
   AddCommand("console",     false, false, &Command::SetActiveAndVisible<Ui::Screen::Console>);
   AddCommand("help",        true,  false, &Command::SetActiveAndVisible<Ui::Screen::Help>);
   AddCommand("library",     true,  false, &Command::SetActiveAndVisible<Ui::Screen::Library>);
   AddCommand("directory",   true,  false, &Command::SetActiveAndVisible<Ui::Screen::Directory>);
   AddCommand("playlist",    true,  false, &Command::SetActiveAndVisible<Ui::Screen::Playlist>);
   AddCommand("outputs",     true,  false, &Command::SetActiveAndVisible<Ui::Screen::Outputs>);
   AddCommand("lists",       true,  false, &Command::SetActiveAndVisible<Ui::Screen::Lists>);
   AddCommand("windowselect",false, false, &Command::SetActiveAndVisible<Ui::Screen::WindowSelect>);

   AddCommand("load",       true,  false, &Command::LoadPlaylist);
   AddCommand("save",       true,  false, &Command::SavePlaylist);
   AddCommand("e",          true,  false, &Command::LoadPlaylist);
   AddCommand("edit",       true,  false, &Command::LoadPlaylist);
   AddCommand("w",          true,  false, &Command::SavePlaylist);
   AddCommand("write",      true,  false, &Command::SavePlaylist);
   AddCommand("toplaylist", true,  false, &Command::ToPlaylist);

   // Local socket commands
   AddCommand("localadd",   true,  true,  &Command::AddFile);

#ifdef __DEBUG_PRINTS
   AddCommand("debug-console",       false, false, &Command::SetActiveAndVisible<Ui::Screen::DebugConsole>);
   AddCommand("debug-client-getmeta",true,  false, &Command::DebugClient<&Mpc::Client::GetAllMetaInformation>);
#endif

#ifdef TEST_ENABLED
   AddCommand("test-console",       false, false, &Command::SetActiveAndVisible<Ui::Screen::TestConsole>);
   AddCommand("test",               false, false, &Command::Test);
   AddCommand("test-screen",        false, false, &Command::TestScreen);
   AddCommand("test-resize",        false, false, &Command::TestResize);
   AddCommand("test-input-random",  true,  false, &Command::TestInputRandom);
   AddCommand("test-input-seq",     true,  false, &Command::TestInputSequence);
#endif

   // Add all settings to command table to provide tab completion
   for (auto setting : settings_.AvailableSettings())
   {
      settingsTable_.push_back("set " + setting);
   }

   // Register for events
   Main::Vimpc::EventHandler(Event::Connected, [this] (EventData const & Data) { ExecuteQueuedCommands(); });
}

Command::~Command()
{
   Running = false;

#ifdef HAVE_TEST_H
   testThread_.join();
#endif
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
      inputString_  = TabComplete(inputString_);
      inputWString_ = stringtow(inputString_);
   }

   InputMode::GenerateInputString(input);
}


void Command::AddCommand(std::string const & name, bool requiresConnection, bool hasRangeSupport, CommandFunction command)
{
   commandTable_[name]       = command;
   requiresConnection_[name] = requiresConnection;
   supportsRange_[name]      = hasRangeSupport;
}

bool Command::ExecuteCommand(std::string const & input)
{
   Regex::RE const blankCommand("^\\s*$");
   Regex::RE const multipleCommands("^(\\s*([^;]+)\\s*;\\s*).*$");
   Regex::RE const rangeSplit("(^\\d*),?(\\d*)$");

   std::string matchString, commandString;
   std::string range, command, arguments;
   uint32_t    count = 0;
   uint32_t    line  = 0; //Actually stores line + 1, as 0 represents invalid

   SplitCommand(input, range, command, arguments);

   std::string fullCommand(command + " " + arguments);

   // Resolve a command from an alias
   if (aliasTable_.find(command) != aliasTable_.end())
   {
      fullCommand = aliasTable_[command] + " " + arguments;
   }

   // Determine whether a range or a line number was used of the form
   // :x or :x,y this determines where the command should start
   // and how many times it should run
   if (range != "")
   {
      std::string rangeLine, rangeCount;
      rangeSplit.Capture(range.c_str(), &rangeLine, &rangeCount);

      if ((Algorithm::isNumeric(rangeLine) == true) && (rangeLine != ""))
      {
         // Commands of the form :<number> go to that line instead
         int32_t lineint = atoi(rangeLine.c_str());
         line = (lineint > 1) ? lineint : 1;

         if ((Algorithm::isNumeric(rangeCount) == true) && (rangeCount != ""))
         {
            int32_t intcount = atoi(rangeCount.c_str());

            if (intcount >= lineint)
            {
               count = (intcount <= 0) ? 0 : static_cast<uint32_t>((intcount - lineint) + 1);
            }
            else
            {
               count = (intcount <= 0) ? 0 : static_cast<uint32_t>((lineint - intcount) + 1);
               line = (intcount > 1) ? intcount : 1;
            }
         }
      }
   }

   // If there are multiple commands we need to run them all
   // But in the case of alias we don't actually want to run the commands
   if ((multipleCommands.Matches(fullCommand) == true) && (command != "alias"))
   {
      // There are multiple commands seperated by ';' we have to handle each one
      while (multipleCommands.Capture(fullCommand, &matchString, &commandString) == true)
      {
         fullCommand = fullCommand.substr(matchString.size());

         if (blankCommand.Matches(commandString) == false)
         {
            ExecuteCommand(commandString);
         }
      }

      // The last command does not need a ';'
      if (blankCommand.Matches(fullCommand) == false)
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
   else if (command != "")
   {
      // Just a normal command
      Debug("Executing command :%u,%u %s %s", line, count, command.c_str(), arguments.c_str());
      ExecuteCommand(line, count, command, arguments);
   }
   else if (line > 0)
   {
      screen_.ScrollTo(line - 1);

      if (count > 0)
      {
         screen_.ScrollTo(line + count - 2);
      }
   }

   return true;
}

void Command::SetQueueCommands(bool enabled)
{
   queueCommands_ = enabled;
}


void Command::ExecuteQueuedCommands()
{
   for (auto command : commandQueue_)
   {
      Debug("Executing queued command :%u,%u %s %s", command.line, command.count, command.command.c_str(), command.arguments.c_str());
      ExecuteCommand(command.line, command.count, command.command, command.arguments);
   }

   commandQueue_.clear();
}

bool Command::RequiresConnection(std::string const & command)
{
   return requiresConnection_[command];
}

bool Command::SupportsRange(std::string const & command)
{
   return supportsRange_[command];
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
   std::vector<std::string> const args = SplitArguments(arguments);

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
      int32_t SongId = atoi(args[0].c_str()) - 1;
      SongId = (SongId == -1) ? 0 : SongId;

      if (SongId >= 0)
      {
         Player::Play(SongId);
      }
      else
      {
         ErrorString(ErrorNumber::InvalidParameter, "invalid song id");
      }
   }
   else
   {
      ErrorString(ErrorNumber::InvalidParameter, "too many parameters");
   }
}

bool Command::CheckConnected()
{
   if (clientState_.Connected() == false)
   {
      ErrorString(ErrorNumber::ClientNoConnection);
      return false;
   }

   return true;
}

void Command::Add(std::string const & arguments)
{
   if (CheckConnected() == true)
   {
      screen_.Initialise(Ui::Screen::Playlist);

      if (arguments != "")
      {
         // Add based on a URI
         client_.Add(arguments);
      }
      else
      {
         // Add according to current selection or range
         screen_.ActiveWindow().AddLine(screen_.ActiveWindow().CurrentLine(), count_, false);
      }

      client_.AddComplete();
   }
}

void Command::AddFile(std::string const & arguments)
{
   if (arguments != "")
   {
      // Add a local filesystem song
      Add("file://" + arguments);
   }
   else
   {
      Add("");
   }
}

void Command::AddAll(std::string const & arguments)
{
   if (CheckConnected() == true)
   {
      screen_.Initialise(Ui::Screen::Playlist);
      client_.AddAllSongs();
      client_.AddComplete();
   }
}

void Command::Delete(std::string const & arguments)
{
   if (CheckConnected() == true)
   {
      screen_.Initialise(Ui::Screen::Playlist);

      std::vector<std::string> const args = SplitArguments(arguments);

      if (args.size() == 2)
      {
         // Delete a range of songs from pos1 to pos2
         uint32_t pos1 = atoi(args[0].c_str()) - 1;
         uint32_t pos2 = atoi(args[1].c_str()) - 1;

         client_.Delete(pos1, pos2 + 1);
      }
      else if (args.size() == 1)
      {
         // Delete the song at given position
         client_.Delete(atoi(args[0].c_str()) - 1);
      }
      else
      {
         // Delete selected or range
         screen_.ActiveWindow().DeleteLine(screen_.ActiveWindow().CurrentLine(), count_, false);
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
   int min = 0, sec = 0;
   size_t const pos = arguments.find_first_of(":");

   if (pos != std::string::npos)
   {
      std::string minutes = arguments.substr(0, pos);
      std::string seconds = arguments.substr(pos + 1);
      min = atoi(minutes.c_str());
      sec = atoi(seconds.c_str());
   }
   else
   {
      sec = atoi(arguments.c_str());
   }
   
   if (min < 0 || sec < 0)
   {
      ErrorString(ErrorNumber::InvalidParameter);
   }
   else
   {
      time = static_cast<uint32_t>( (min * 60) + sec );
      Player::Seek(Delta * time);
   }
}

void Command::SeekTo(std::string const & arguments)
{
   uint32_t time = 0;
   int min = 0, sec = 0;
   size_t const pos = arguments.find_first_of(":");

   if (pos != std::string::npos)
   {
      std::string minutes = arguments.substr(0, pos);
      std::string seconds = arguments.substr(pos + 1);
      min = atoi(minutes.c_str()); // use int type so we can look for a 
                                   // negative seek value
      sec = atoi(seconds.c_str());
   }
   else
   {
      sec = atoi(arguments.c_str());
   }

   if (min < 0 || sec < 0)
   {
      ErrorString(ErrorNumber::InvalidParameter);
   }
   else
   {
      time = static_cast<uint32_t>( (min * 60) + sec );
      Player::SeekTo(time);
   }
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
   if ((forceCommand_ == true) ||
       (settings_.Get(Setting::StopOnQuit) == true))
   {
      Player::Stop();
   }

   Player::Quit();
}

void Command::Mute(std::string const & arguments)
{
   if (CheckConnected() == true)
   {
      if (arguments != "")
      {
         client_.SetMute((arguments.compare("on") == 0));
      }
   }
}

void Command::Volume(std::string const & arguments)
{
   uint32_t const Vol = static_cast<uint32_t>(atoi(arguments.c_str()));

   if (Vol <= 100)
   {
      Player::Volume(Vol);
   }
   else
   {
      ErrorString(ErrorNumber::InvalidParameter);
   }
}


bool Command::ConnectionAttempt()
{
   return connectAttempt_;
}

void Command::Connect(std::string const & arguments)
{
   // Ignore the connect command when starting up if -h/-p used
   // on the command line
   if (settings_.SkipConfigConnects() == false)
   {
      size_t   pos  = arguments.find_first_of(" ");
      uint32_t port = 0;

      std::string hostname = arguments.substr(0, pos);

      if (pos != std::string::npos)
      {
         port = atoi(arguments.substr(pos + 1).c_str());
      }

      client_.Connect(hostname, port);

      connectAttempt_ = true;
   }
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
   if (password == "")
   {
      screen_.PromptForPassword();
   }
   else
   {
      client_.Password(password.c_str());
   }
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

void Command::Substitute(std::string const & expression)
{
#ifdef TAG_SUPPORT
   typedef void (*EditFunction)(Mpc::Song *, std::string const &, char const *);
   typedef std::map<std::string, EditFunction> OptionsMap;

   typedef std::string const & (Mpc::Song::*ReadFunction)() const;
   typedef std::map<std::string, ReadFunction> InfoMap;

   static OptionsMap modifyFunctions;
   static InfoMap    readFunctions;

   if (modifyFunctions.size() == 0)
   {
      modifyFunctions["a"] = &Mpc::Tag::SetArtist;
      modifyFunctions["b"] = &Mpc::Tag::SetAlbum;
      modifyFunctions["t"] = &Mpc::Tag::SetTitle;
      modifyFunctions["n"] = &Mpc::Tag::SetTrack;

      readFunctions["a"] = &Mpc::Song::Artist;
      readFunctions["b"] = &Mpc::Song::Album;
      readFunctions["t"] = &Mpc::Song::Title;
      readFunctions["n"] = &Mpc::Song::Track;
   }

   std::string match, substitution, options;

   if (settings_.Get(Setting::LocalMusicDir) != "")
   {
      for (uint32_t i = 0; i < count_; ++i)
      {
         Mpc::Song * const song = screen_.GetSong(screen_.ActiveWindow().CurrentLine() + i);

         if (song != NULL)
         {
            std::string path = settings_.Get(Setting::LocalMusicDir) + "/" + song->URI();

            Regex::RE const split("^/([^/]*)/([^/]*)/([^/]*)\n?$");
            split.Capture(expression.c_str(), &match, &substitution, &options);

            if (options.empty() == false)
            {
               InfoMap::iterator    it = readFunctions.find(options);
               OptionsMap::iterator jt = modifyFunctions.find(options);

               if (it != readFunctions.end())
               {
                  ReadFunction readFunction = it->second;

                  if (match == "")
                  {
                     match = ".*";
                  }

                  Regex::RE const check(match);
                  std::string value = ((*song).*readFunction)();

                  if (check.Matches(value) == true)
                  {
                     if (jt != modifyFunctions.end())
                     {
                        check.Replace(substitution, value);
                        EditFunction editFunction = jt->second;
                        (*editFunction)(song, path, value.c_str());

                        if (settings_.Get(Setting::AutoUpdate) == true)
                        {
                           client_.Update(song->URI());
                        }
                     }
                  }
               }
            }
         }
      }
   }
   else
   {
      ErrorString(ErrorNumber::NotSet, "local-music-dir");
   }
#endif
}


template <bool ON>
void Command::Output(std::string const & arguments)
{
   int32_t output = -1;
   bool    rangeAllowed = false;

   screen_.Initialise(Ui::Screen::Outputs);

   if (arguments == "")
   {
      output = screen_.GetSelected(Ui::Screen::Outputs);
      rangeAllowed = true;
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
      if (rangeAllowed == false)
      {
         client_.SetOutput(Main::Outputs().Get(output), (ON == true));
      }
      else
      {
         for (int i = 0; (i < static_cast<int>(count_)); ++i)
         {
            if ((output + i < static_cast<int32_t>(Main::Outputs().Size())) && (output + i >= 0))
            {
               client_.SetOutput(Main::Outputs().Get(output + i), (ON == true));
            }
         }
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
   bool    rangeAllowed = false;

   screen_.Initialise(Ui::Screen::Outputs);

   if (arguments == "")
   {
      // Toggle selected or range of outputs
      output = screen_.GetSelected(Ui::Screen::Outputs);
      rangeAllowed = true;
   }
   else if (Algorithm::isNumeric(arguments.c_str()) == true)
   {
      // Toggle a given output number
      output = atoi(arguments.c_str());
   }
   else
   {
      // Toggle based on the output name
      output = Player::FindOutput(arguments);
   }

   if ((output < static_cast<int32_t>(Main::Outputs().Size())) && (output >= 0))
   {
      if (rangeAllowed == false)
      {
         Player::ToggleOutput(output);
      }
      else
      {
         for (uint32_t i = 0; (i < count_); ++i)
         {
            if (output + i < Main::Outputs().Size())
            {
               Player::ToggleOutput(output + i);
            }
         }
      }
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
      client_.SavePlaylist(arguments);
   }
   else if (client_.HasLoadedPlaylist())
   {
      client_.SaveLoadedPlaylist();
   }
   else
   {
      ErrorString(ErrorNumber::NoPlaylistLoaded);
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
      client_.AddAllSearchResults();
   }
   else
   {
      client_.SearchResults(arguments);
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

void Command::PrintMappings(std::string tabname)
{
   Ui::Normal::MapNameTable mappings = normalMode_.Mappings();

   if (tabname != "")
   {
      mappings = normalMode_.WindowMappings(screen_.GetWindowFromName(tabname));
   }

   if (mappings.size() > 0)
   {
      PagerWindow * const pager = screen_.GetPagerWindow();
      pager->Clear();

      for (auto mapping : mappings)
      {
         pager->AddLine(mapping.first + "   " + mapping.second);
      }

      screen_.ShowPagerWindow();
   }
   else
   {
      ErrorString(ErrorNumber::NoSuchMapping);
   }
}


void Command::Map(std::string const & arguments)
{
   if ((arguments.find(" ") != std::string::npos))
   {
      std::string const key     = arguments.substr(0, arguments.find(" "));
      std::string const mapping = arguments.substr(arguments.find(" ") + 1);
      normalMode_.Map(key, mapping);
   }
   else if (arguments == "")
   {
      PrintMappings();
   }
}

void Command::Unmap(std::string const & arguments)
{
   normalMode_.Unmap(arguments);
}

void Command::TabMap(std::string const & arguments)
{
   std::string range, tabname, args;
   SplitCommand(arguments, range, tabname, args);

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
      PrintMappings(tabname);
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

   if ((arguments.find(" ") != std::string::npos))
   {
      int32_t position1 = atoi(arguments.substr(0, arguments.find(" ")).c_str());
      int32_t position2 = atoi(arguments.substr(arguments.find(" ") + 1).c_str());

      if (position1 > static_cast<int32_t>(screen_.ActiveWindow().BufferSize() - 1))
      {
         position1 = Main::Playlist().Size();
      }
      else if (position1 <= 1)
      {
         position1 = 1;
      }

      if (position2 > static_cast<int32_t>(screen_.ActiveWindow().BufferSize() - 1))
      {
         position2 = Main::Playlist().Size();
      }
      else if (position2 <= 1)
      {
         position2 = 1;
      }

      if ((Main::Playlist().Size() > 0) &&
          (position1 <= static_cast<int32_t>(Main::Playlist().Size())) &&
          (position2 <= static_cast<int32_t>(Main::Playlist().Size())))
      {
         client_.Move(position1 - 1, position2 - 1);

         Mpc::Song * song = Main::Playlist().Get(position1 - 1);
         Main::Playlist().Remove(position1 - 1, 1);
         Main::Playlist().Add(song, position2 - 1);
         screen_.Update();
      }
      else
      {
         // \TODO error here!
      }
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

   if ((arguments.find(" ") != std::string::npos))
   {
      std::string const position1 = arguments.substr(0, arguments.find(" "));
      std::string const position2 = arguments.substr(arguments.find(" ") + 1);
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
   if (currentLine_ != -1)
   {
      screen_.ScrollTo(currentLine_);
   }

   screen_.SetActiveAndVisible(static_cast<int32_t>(MAINWINDOW));

   if (line_ != -1)
   {
      screen_.ScrollTo(line_);
   }
}

template <Command::Location LOCATION>
void Command::ChangeToWindow(std::string const & arguments)
{
   int32_t active = 0;

   switch (LOCATION)
   {
      case First:
         active = 0;
         break;

      case Next:
         active = ((screen_.GetActiveWindowIndex() + 1) %
                   screen_.VisibleWindows());
         break;

      case Previous:
         active = screen_.GetActiveWindowIndex() - 1;
         active = (active < 0) ? screen_.VisibleWindows() - 1 : active;
         break;

      case Last:
         active = screen_.VisibleWindows() - 1;
         break;

      case LocationCount:
      default:
         break;
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
   if ((arguments.find(" ") != std::string::npos))
   {
      std::string oldname = arguments.substr(0, arguments.find(" "));
      std::string newname = arguments.substr(arguments.find(" ") + 1);

      int32_t id = screen_.GetWindowFromName(oldname);

      if (id != Ui::Screen::Unknown)
      {
         if (screen_.GetWindowFromName(newname) == Ui::Screen::Unknown)
         {
            screen_.Window(id).SetName(newname);
         }
         else
         {
            ErrorString(ErrorNumber::NameInUse, newname);
         }
      }
      else
      {
         ErrorString(ErrorNumber::TabDoesNotExist, oldname);
      }
   }
   else
   {
      if (screen_.GetWindowFromName(arguments) == Ui::Screen::Unknown)
      {
         screen_.ActiveWindow().SetName(arguments);
      }
      else
      {
         ErrorString(ErrorNumber::NameInUse, arguments);
      }
   }
}

void Command::SetColour(std::string const & arguments)
{
   if ((arguments.find(" ") != std::string::npos))
   {
      std::string oldname = arguments.substr(0, arguments.find(" "));
      std::string newname = arguments.substr(arguments.find(" ") + 1);

      settings_.SetColour(oldname, newname);
   }
   else
   {
      ErrorString(ErrorNumber::NoParameter, arguments);
   }
}

template <Ui::Command::ClientFunction FUNC>
void Command::DebugClient(std::string const & arguments)
{
   Ui::Command::ClientFunction func = FUNC;
   (client_.*func)();
}

void Command::TestExecutor()
{
   while (Running == true)
   {
      UniqueLock<Mutex> Lock(QueueMutex);

      if ((Queue.empty() == false) ||
          (ConditionWait(Condition, Lock, 250) != false))
      {
         if (Queue.empty() == false)
         {
            std::string arguments = Queue.front();
            Queue.pop_front();
            Lock.unlock();

#ifdef HAVE_TEST_H
            Main::Tester::Instance().Vimpc->HandleUserEvents(false);

            CPPUNIT_NS::TestResult testresult;
            CPPUNIT_NS::TestResultCollector collectedresults;
            testresult.addListener(&collectedresults);
            CPPUNIT_NS::TestRunner testrunner;

            if ((arguments == "") || (arguments == "all"))
            {
               Main::TestConsole().Add("Running all tests...");
               testrunner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
            }
            else
            {
               Main::TestConsole().Add("Running test " + arguments + "...");
               CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry(arguments);
               testrunner.addTest(registry.makeTest());
            }
            testrunner.run(testresult);

            std::stringstream outStream;
            CPPUNIT_NS::TextOutputter textoutput(&collectedresults, outStream);
            textoutput.write();

            std::string output;

            while (!outStream.eof())
            {
               std::getline(outStream, output);

               if (output != "")
               {
                  EventData Data; Data.name = output;
                  Main::Vimpc::CreateEvent(Event::TestResult, Data);
                  Main::Vimpc::CreateEvent(Event::Repaint,   Data);
               }
            }

            Main::Tester::Instance().Vimpc->HandleUserEvents(true);
   #endif
         }
      }
   }
}

void Command::Test(std::string const & arguments)
{
   UniqueLock<Mutex> Lock(QueueMutex);
   Queue.push_back(arguments);
   Condition.notify_all();
}

void Command::TestInputRandom(std::string const & arguments)
{
   int count = atoi(arguments.c_str());
   screen_.EnableRandomInput(count);
}

void Command::TestInputSequence(std::string const & arguments)
{
   for (int i = arguments.size() - 1; i >= 0; --i)
   {
      ungetch(arguments[i]);
   }
}

void Command::TestScreen(std::string const & arguments)
{
   screen_.ActiveWindow().ScrollTo(65535);
   screen_.ScrollTo(screen_.ActiveWindow().Playlist(0));
   screen_.ScrollTo(screen_.ActiveWindow().Current());
   screen_.ActiveWindow().ScrollTo(0);
   screen_.Invalidate(screen_.GetActiveWindow());
}

void Command::TestResize(std::string const & arguments)
{
   std::vector<std::string> args = SplitArguments(arguments);

   if (args.size() == 2)
   {
      screen_.Resize(true, atoi(args[0].c_str()), atoi(args[1].c_str()));
   }
}


bool Command::ExecuteCommand(uint32_t line, uint32_t count, std::string command, std::string const & arguments)
{
   Regex::RE const forceCheck("^.*!$");

   forceCommand_ = (forceCheck.Matches(command));

   if (forceCommand_ == true)
   {
      command = command.substr(0, command.length() - 1);
   }

   // If we can't find the exact command, look for a unique command that starts
   // with the input command
   std::string commandToExecute  = command;
   bool        matchingCommand   = (commandTable_.find(commandToExecute) != commandTable_.end());
   uint32_t    validCommandCount = 0;

   if (matchingCommand == false)
   {
      for (auto cmd : commandTable_)
      {
         if (command.compare(cmd.first.substr(0, command.length())) == 0)
         {
            ++validCommandCount;
            commandToExecute = cmd.first;
         }
      }

      matchingCommand = (validCommandCount == 1);
   }

   // If we have found a command execute it, with \p arguments
   if (matchingCommand == true)
   {
      // This is a hack for ranges on the setactive command
      // so that we return the scroll for the window before the command
      // is executed and change it for the new one
      currentLine_ = -1;
      line_        = -1;

      // If a range was specified and supported scroll to the line
      // corresponding to the first part of the range
      // \TODO this may break if it is done when we are in visual mode
      if ((SupportsRange(commandToExecute) == true) && (line > 0))
      {
         currentLine_ = screen_.GetActiveSelected();
         line_        = line - 1;
         screen_.ScrollTo(line - 1);
      }
      else if (line > 0)
      {
         ErrorString(ErrorNumber::NoRangeAllowed, command);
         return true;
      }

      if ((RequiresConnection(commandToExecute) == false) || (queueCommands_ == false) || (clientState_.Connected() == true))
      {
         count_ = (count <= 0) ? 1 : count;
         CommandTable::const_iterator const it = commandTable_.find(commandToExecute);
         CommandFunction const commandFunction = it->second;
         (*this.*commandFunction)(arguments);
      }
      else if ((RequiresConnection(commandToExecute) == true) && ((queueCommands_ == true) && (clientState_.Connected() == false)))
      {
         CommandArgs commandArgs;
         commandArgs.line      = line;
         commandArgs.count     = count;
         commandArgs.command   = commandToExecute;
         commandArgs.arguments = arguments;
         commandQueue_.push_back(commandArgs);
      }

      currentLine_ = -1;
      line_        = -1;

      if ((SupportsRange(commandToExecute) == true) && (count > 0) && (line > 0))
      {
         screen_.ScrollTo(line + count - 2);
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

void Command::SplitCommand(std::string const & input, std::string & range, std::string & command, std::string & arguments)
{
   Regex::RE const split("^([\\d,]*)?([^ /]*) ?(/?[^\n]*)\n?$");
   split.Capture(input.c_str(), &range, &command, &arguments);
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
      PagerWindow * pager = screen_.GetPagerWindow();
      pager->Clear();

      for (auto setting : settings_.AvailableSettings())
      {
         if (((setting.size() < 2) || (setting.substr(0, 2) != "no")) &&
             (settings_.Get<bool>(setting) == true))
         {
            pager->AddLine(setting);
         }
         else if (settings_.Get<std::string>(setting) != "")
         {
            std::string const value = settings_.Get<std::string>(setting);
            pager->AddLine(setting + "=" + value);
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
   snprintf(port, 8, "%u", clientState_.Port());

   // Ensure that we use the same mpd_host and port for mpc that
   // we are using but still allow the person running the command
   // to do -h and -p flags
   std::string const command("MPD_HOST=" + clientState_.Hostname() + " MPD_PORT=" + std::string(port) +
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
   std::string range, command, arguments;

   SplitCommand(input, range, command, arguments);

   aliasTable_[command] = arguments;
}

void Command::Unalias(std::string const & input)
{
   std::string range, command, arguments;

   SplitCommand(input, range, command, arguments);

   if (aliasTable_.find(command) != aliasTable_.end())
   {
      aliasTable_.erase(command);
   }
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
      addTable_.clear();
      loadTable_.clear();

      screen_.Initialise(Ui::Screen::Directory);
      screen_.Initialise(Ui::Screen::Lists);

      for (uint32_t i = 0; i < Main::MpdLists().Size(); ++i)
      {
         loadTable_.push_back("load " + Main::MpdLists().Get(i).name_);
         loadTable_.push_back("edit " + Main::MpdLists().Get(i).name_);
      }

      std::vector<std::string> const & paths = Main::Directory().Paths();

      for (uint32_t i = 0; i < paths.size(); ++i)
      {
         addTable_.push_back("add " + paths[i]);
      }
   }

   if (tabStart.find("set ") == 0)
   {
      return TabComplete(tabStart, settingsTable_, TabCompletionMatch<std::string>(tabStart));
   }
   else if ((tabStart.find("load ") == 0) || (tabStart.find("edit ") == 0))
   {
      return TabComplete(tabStart, loadTable_, TabCompletionMatch<std::string>(tabStart));
   }
   else if (tabStart.find("add ") == 0)
   {
      return TabComplete(tabStart, addTable_, TabCompletionMatch<std::string>(tabStart));
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
