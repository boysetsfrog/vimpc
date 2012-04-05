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

#include "inputmode.hpp"
#include "player.hpp"

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

   public:
      Command(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings, Ui::Search & search, Ui::Normal & normalMode);
      ~Command();

   public:
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

   public: // Ui::InputMode
      void Initialise(int input);
      bool Handle(int input);
      void GenerateInputString(int const input);

   private: //Ui::InputMode
      bool InputStringHandler(std::string input);
      char const * Prompt() const;

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

      //! Specify a password to mpd
      void Password(std::string const & password);

      //! Echos a string to the console window
      //!
      //! \param echo The string to be echoed
      void Echo(std::string const & echo);

      //! Similar to echo but in the errror window
      void EchoError(std::string const & echo);

   private:
      void Add(std::string const & arguments);
      void Delete(std::string const & arguments);
      void DeleteAll(std::string const & arguments);

      template <int Delta>
      void Seek(std::string const & arguments);
      void SeekTo(std::string const & arguments);

   private:
      template <bool ON>
      void Output(std::string const & arguments);

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

      void Map(std::string const & arguments);
      void Unmap(std::string const & arguments);

      void Rescan(std::string const & arguments);
      void Update(std::string const & arguments);

   private: //Ui::Player
      template <Player::Skip SKIP>
      void SkipSong(std::string const & arguments);

   private: // Screen related functionality
      template <Ui::Screen::MainWindow MAINWINDOW>
      void SetActiveAndVisible(std::string const & arguments);

      typedef enum { First, Last, LocationCount } Location;

      template <Location LOCATION>
      void ChangeToWindow(std::string const & arguments);

      void HideWindow(std::string const & arguments);
      void MoveWindow(std::string const & arguments);
      void RenameWindow(std::string const & arguments);

   private:
      // Executes \p command using \p arguments
      //
      // \param[in] command   The command to execute
      // \param[in] arguments The arguments to pass to the command
      bool ExecuteCommand(std::string command, std::string const & arguments);

      // Splits the input into command and argument parts
      //
      // \param[in]  input     The string to split
      // \param[out] command   The command part of the string
      // \param[out] arguments The arguments from the string
      void SplitCommand(std::string const & input, std::string & command, std::string & arguments);

      // Handle the settings
      void Set(std::string const & arguments);

      // Call the cli mpc client
      void Mpc(std::string const & arguments);

      // Alias a command to a given string
      void Alias(std::string const & arguments);

      // Clears the current tab completion
      void ResetTabCompletion(int input);

      // Complete the current command, by searching through the command table
      // for commands that begin with the currently set command
      std::string TabComplete(std::string const & command);

   private:
      typedef void (Ui::Command::*CommandFunction)(std::string const &);
      typedef std::map<std::string, std::string>     AliasTable;
      typedef std::map<std::string, CommandFunction> CommandTable;

      typedef std::pair<std::string, std::string>    CommandArgPair;
      typedef std::vector<CommandArgPair>            CommandQueue;

   private:
      bool                 initTabCompletion_;
      bool                 forceCommand_;
      bool                 queueCommands_;
      AliasTable           aliasTable_;
      CommandTable         commandTable_;
      CommandQueue         commandQueue_;
      Ui::Search         & search_;
      Ui::Screen         & screen_;
      Mpc::Client        & client_;
      Main::Settings     & settings_;
      Ui::Normal         & normalMode_;

      // Tab completion searching class
      class TabCompletionMatch
      {
      public:
         TabCompletionMatch(std::string const & key) :
            key_(key)
         {}

      public:
         bool operator() (std::pair<std::string, Ui::Command::CommandFunction> element)
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
/* vim: set sw=3 ts=3: */
