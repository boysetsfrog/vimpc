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

#include "compiler.hpp"
#include "inputmode.hpp"
#include "player.hpp"
#include "test.hpp"

namespace Main
{
   class Settings;
}

namespace Ui
{
   class ConsoleWindow;
   class Normal;

   // Handles all input received whilst in command mode
   class Command : public InputMode, public Player
   {
   private:
      typedef void (Mpc::Client::*ClientFunction)();
      typedef void (Ui::Command::*CommandFunction)(std::string const &);

   public:
      Command(Main::Vimpc * vimpc, Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Main::Settings & settings, Ui::Search & search, Ui::Normal & normalMode);
      ~Command();

   public:
      // Add a new command to the table
      void AddCommand(std::string const & name, bool requiresConnection, bool hasRangeSupport, CommandFunction command);

      // Checks if there is any aliases defined for a command, recursively calling
      // until a proper command is found then executes that command
      //
      // \param[in] input The command string to execute, including the arguments
      bool ExecuteCommand(std::string const & input);

      // If command queue'ing is enabled, commands that require an active mpd connection
      // will be queued up if a connection is not present, then when a connection becomes
      // active they will be handled
      void SetQueueCommands(bool enabled);

      // Execute all the commands which have been queued up
      void ExecuteQueuedCommands();

      // Returns true if the given command requires an mpd connection
      bool RequiresConnection(std::string const & command);

      // Returns true if the command can be used over a range
      bool SupportsRange(std::string const & command);

      // Whether a connect command has been run
      bool ConnectionAttempt();

   public: // Ui::InputMode
      void Initialise(int input);
      bool Handle(int input);
      void GenerateInputString(int const input);

   private: //Ui::InputMode
      bool InputStringHandler(std::string input);
      char const * Prompt() const;
      bool CheckConnected();

   private: //Ui::Player wrapper functions
      void ClearScreen(std::string const & arguments);
      void Pause(std::string const & arguments);
      void Play(std::string const & arguments);
      void Quit(std::string const & arguments);
      void QuitAll(std::string const & arguments);
      void Random(std::string const & arguments);
      void Repeat(std::string const & arguments);
      void Single(std::string const & arguments);
      void Consume(std::string const & arguments);
      void Crossfade(std::string const & arguments);
      void Move(std::string const & arguments);
      void Shuffle(std::string const & arguments);
      void Swap(std::string const & arguments);
      void Redraw(std::string const & arguments);
      void Stop(std::string const & arguments);
      void Mute(std::string const & arguments);
      void Volume(std::string const & arguments);

   // Command only functions
   private:
      //! Connects the mpd client to the given host
      //!
      //! \param host The hostname to connect to
      //! \param port The port to connect with
      void Connect(std::string const & arguments);
      void Disconnect(std::string const & arguments);
      void Reconnect(std::string const & arguments);

      void NoHighlightSearch(std::string const & arguments);

      //! Execute the input as a normal mode command
      void Normal(std::string const & arguments);

      //! Specify a password to mpd
      void Password(std::string const & password);

      //! Echos a string to the console window
      //!
      //! \param echo The string to be echoed
      void Echo(std::string const & echo);

      //! Similar to echo but in the errror window
      void EchoError(std::string const & echo);

      void Sleep(std::string const & expression);

      void Substitute(std::string const & seconds);

   private:
      void Add(std::string const & arguments);
      void AddAll(std::string const & arguments);
      void Delete(std::string const & arguments);
      void DeleteAll(std::string const & arguments);

      void AddFile(std::string const & arguments);

      template <int Delta>
      void Seek(std::string const & arguments);
      void SeekTo(std::string const & arguments);

   private:
      template <bool ON>
      void Output(std::string const & arguments);
      void ToggleOutput(std::string const & arguments);

   private:
      void LoadPlaylist(std::string const & arguments);
      void SavePlaylist(std::string const & arguments);
      void ToPlaylist(std::string const & arguments);

      void Find(std::string const & arguments);
      void FindAny(std::string const & arguments);
      void FindAlbum(std::string const & arguments);
      void FindArtist(std::string const & arguments);
      void FindGenre(std::string const & arguments);
      void FindSong(std::string const & arguments);

      void PrintMappings(std::string tabname = "");
      void Map(std::string const & arguments);
      void Unmap(std::string const & arguments);
      void TabMap(std::string const & arguments);
      void TabMap(std::string const & tabname, std::string const & arguments);
      void TabUnmap(std::string const & arguments);
      void WindowMap(std::string const & arguments);
      void WindowUnmap(std::string const & arguments);

      void Rescan(std::string const & arguments);
      void Update(std::string const & arguments);

   private: //Ui::Player
      template <Player::Skip SKIP>
      void SkipSong(std::string const & arguments);

   private: // Screen related functionality
      template <Ui::Screen::MainWindow MAINWINDOW>
      void SetActiveAndVisible(std::string const & arguments);

      typedef enum { First, Last, Next, Previous, LocationCount } Location;

      template <Location LOCATION>
      void ChangeToWindow(std::string const & arguments);

      void HideWindow(std::string const & arguments);
      void MoveWindow(std::string const & arguments);
      void RenameWindow(std::string const & arguments);

   private: // Colour configuration
      void SetColour(std::string const & arguments);

   private: // Debug only commands
      template <ClientFunction FUNC>
      void DebugClient(std::string const & arguments);

   private: // Test only commands
      void TestExecutor();
      void Test(std::string const & arguments);
      void TestInputRandom(std::string const & arguments);
      void TestInputSequence(std::string const & arguments);
      void TestScreen(std::string const & arguments);
      void TestResize(std::string const & arguments);

   protected:
      // Splits the input into command and argument parts
      //
      // \param[in]  input     The string to split
      // \param[out] range     The range to run commands over
      // \param[out] command   The command part of the string
      // \param[out] arguments The arguments from the string
      void SplitCommand(std::string const & input, std::string & range, std::string & command, std::string & arguments);

      // Splits the arguments based on the given delimeter
      // \param[in]  input     The string to split
      // \param[in]  delim     The delimiting character
      std::vector<std::string> SplitArguments(std::string const & input, char delimeter = ' ');

   private:
      // Executes \p command using \p arguments
      //
      // \param[in] command   The command to execute
      // \param[in] arguments The arguments to pass to the command
      bool ExecuteCommand(uint32_t line, uint32_t count, std::string command, std::string const & arguments);

      // Handle the settings
      void Set(std::string const & arguments);

      // Call the cli mpc client
      void Mpc(std::string const & arguments);

      // Alias a command to a given string
      void Alias(std::string const & arguments);
      void Unalias(std::string const & arguments);

      // Clears the current tab completion
      void ResetTabCompletion(int input);

      // Complete the current command, by searching through the command table
      // for commands that begin with the currently set command
      std::string TabComplete(std::string const & command);

   private:
      typedef struct
      {
         uint32_t    line;
         uint32_t    count;
         std::string command;
         std::string arguments;
      } CommandArgs;

   private:
      typedef std::map<std::string, std::string>     AliasTable;
      typedef std::map<std::string, CommandFunction> CommandTable;
      typedef std::vector<CommandArgs>               CommandQueue;
      typedef std::map<std::string, bool>            BoolMap;
      typedef std::vector<std::string>               TabCompTable;

   private:
      bool                 initTabCompletion_;
      bool                 forceCommand_;
      bool                 queueCommands_;
      bool                 connectAttempt_;
      uint32_t             count_;
      int32_t              line_;
      int32_t              currentLine_;
      AliasTable           aliasTable_;
      CommandTable         commandTable_;
      TabCompTable         settingsTable_;
      TabCompTable         loadTable_;
      TabCompTable         addTable_;
      CommandQueue         commandQueue_;
      BoolMap              requiresConnection_;
      BoolMap              supportsRange_;
      Main::Vimpc *        vimpc_;
      Ui::Search         & search_;
      Ui::Screen         & screen_;
      Mpc::Client        & client_;
      Mpc::ClientState   & clientState_;
      Main::Settings     & settings_;
      Ui::Normal         & normalMode_;

#ifdef HAVE_TEST_H
      Thread               testThread_;
#endif

   private:
      // Tab completion searching class
      template <typename T>
      class TabCompletionMatch
      {
      public:
         TabCompletionMatch(std::string const & key) :
            key_(key)
         {}

      public:
         bool operator() (std::pair<std::string, T> element)
         {
            std::string input(element.first);
            return (key_.compare(input.substr(0, key_.length())) == 0);
         }

         bool operator() (std::string element)
         {
            return (key_.compare(element.substr(0, key_.length())) == 0);
         }

      private:
         std::string key_;
      };

      template <typename T, typename U>
      std::string TabComplete(std::string const & tabStart, T const & table, TabCompletionMatch<U> const & completor);
      std::string TabGetCompletion(CommandTable::const_iterator it);
      std::string TabGetCompletion(TabCompTable::const_iterator it);
   };

   }

#endif
/* vim: set sw=3 ts=3: */
