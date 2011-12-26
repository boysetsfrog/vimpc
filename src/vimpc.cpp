/*
   Vimpc
   Copyright (C) 2010 - 2011 Nathan Sweetman

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

   vimpc.hpp - handles mode changes and input processing
   */

#include "vimpc.hpp"

#include "mode/mode.hpp"
#include "mode/normal.hpp"
#include "mode/command.hpp"
#include "mode/search.hpp"

#include "assert.hpp"
#include "buffers.hpp"
#include "config.hpp"
#include "settings.hpp"
#include "window/error.hpp"

#include <sys/time.h>
#include <unistd.h>

using namespace Main;

bool Vimpc::Running = true;

// \todo the coupling and requirements on the way everything needs to be constructed is awful
// this really needs to be fixed and the coupling removed
Vimpc::Vimpc() :
   currentMode_ (Normal),
   settings_    (Main::Settings::Instance()),
   search_      (*(new Ui::Search (screen_, client_, settings_))),
   screen_      (settings_, client_, search_),
   client_      (this, settings_, screen_),
   modeTable_   ()
{
   modeTable_[Command] = new Ui::Command(screen_, client_, settings_);
   modeTable_[Normal]  = new Ui::Normal (screen_, client_, settings_, search_);
   modeTable_[Search]  = &search_;

   ENSURE(modeTable_.size()     == ModeCount);
   ENSURE(ModesAreInitialised() == true);
}

Vimpc::~Vimpc()
{
   for (ModeTable::iterator it = modeTable_.begin(); (it != modeTable_.end()); ++it)
   {
      delete (it->second);
      it->second = NULL;
   }
}

void Vimpc::Run(std::string hostname, uint16_t port)
{
   screen_.Start();

   // Set up the display
   {
      Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);
      mode.Initialise(0);
   }

   SetSkipConfigConnects(hostname != "");

   // Parse the config file
   Ui::Command & commandMode = assert_reference(dynamic_cast<Ui::Command *>(modeTable_[Command]));
   bool const configExecutionResult = Config::ExecuteConfigCommands(commandMode);

   SetSkipConfigConnects(false);

   if (configExecutionResult == true)
   {
      // If we didn't connect to a host from the config file, just connect to the default
      if (client_.Connected() == false)
      {
         client_.Connect(hostname, port);
      }

      client_.DisplaySongInformation();
      screen_.Update();

      // If we still have no connection, report an error
      if (client_.Connected() == false)
      {
         Error(ErrorNumber::ClientNoConnection, "Failed to connect to server, please ensure it is running and type :connect <server> [port]");
      }
      else
      {
         Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);
         mode.Refresh();
      }

      // The main loop
      while (Running == true)
      {
         static long updateTime = 0;

         struct timeval start, end;

         gettimeofday(&start, NULL);
         int input = Input();
         gettimeofday(&end,   NULL);

         long const seconds  = end.tv_sec  - start.tv_sec;
         long const useconds = end.tv_usec - start.tv_usec;
         long const mtime    = (seconds * 1000 + (useconds/1000.0)) + 0.5;

         updateTime += mtime;

         client_.CheckForUpdates();

         if (input != ERR)
         {
            Handle(input);
         }

         if ((input != ERR) || (screen_.Resize() == true) || ((updateTime >= 1000) && (input == ERR)))
         {
            updateTime = 0;
            client_.DisplaySongInformation();

            Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);

            screen_.Update();
            mode.Refresh();
         }
      }
   }
}

Ui::Mode & Vimpc::CurrentMode()
{
   return assert_reference(modeTable_[currentMode_]);
}


/* static */ void Vimpc::SetRunning(bool isRunning)
{
   Running = isRunning;
}


int Vimpc::Input() const
{
   return screen_.WaitForInput();
}

void Vimpc::Handle(int input)
{
   Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);

#ifdef HAVE_MOUSE_SUPPORT
   if (input == KEY_MOUSE)
   {
      HandleMouse();
   }
   else
   {
#endif
      // Input must be handled before mode is changed
      mode.Handle(input);

      if (RequiresModeChange(input) == true)
      {
         ChangeMode(input);
      }
#ifdef HAVE_MOUSE_SUPPORT
   }
#endif
}

void Vimpc::HandleMouse()
{
   screen_.HandleMouseEvent();
}

bool Vimpc::ModesAreInitialised()
{
   bool result = true;

   for (ModeTable::iterator it = modeTable_.begin(); ((it != modeTable_.end()) && (result == true)); ++it)
   {
      result = (it->second != NULL);
   }

   return result;
}


bool Vimpc::RequiresModeChange(int input) const
{
   return (currentMode_ != ModeAfterInput(input));
}

Vimpc::ModeName Vimpc::ModeAfterInput(int input) const
{
   ModeName newMode = currentMode_;

   // Check if we are returning to normal mode
   if (currentMode_ != Normal)
   {
      Ui::Mode const & normalMode = assert_reference(modeTable_.at(Normal));
      Ui::Mode const & commandMode = assert_reference(modeTable_.at(Command));
      Ui::Mode const & searchMode = assert_reference(modeTable_.at(Search));

      if (normalMode.CausesModeToStart(input) == true)
      {
         newMode = Normal;
      }
      //\TODO This reaks
      else if (currentMode_ == Command)
      {
         if (commandMode.CausesModeToEnd(input))
         {
            newMode = Normal;
         }
      }
      else if (currentMode_ == Search)
      {
         if (searchMode.CausesModeToEnd(input))
         {
            newMode = Normal;
         }
      }
   }
   // Must be in normal mode to be able to enter any other mode
   else
   {
      for (ModeTable::const_iterator it = modeTable_.begin(); (it != modeTable_.end()); ++it)
      {
         Ui::Mode const & mode = assert_reference(it->second);

         if (mode.CausesModeToStart(input) == true)
         {
            newMode = it->first;
         }
      }
   }

   return newMode;
}

void Vimpc::ChangeMode(int input)
{
   ModeName const oldModeName = currentMode_;
   ModeName const newModeName = ModeAfterInput(input);

   if (newModeName != oldModeName)
   {
      currentMode_ = newModeName;

      Ui::Mode & oldMode = assert_reference(modeTable_[oldModeName]);
      Ui::Mode & newMode = assert_reference(modeTable_[newModeName]);

      oldMode.Finalise(input);
      newMode.Initialise(input);
   }
}

void Vimpc::SetSkipConfigConnects(bool val)
{
   settings_.SetSkipConfigConnects(val);
}


/* vim: set sw=3 ts=3: */
