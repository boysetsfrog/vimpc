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

   vimpc.hpp - handles mode changes and input processing
   */

#include "vimpc.hpp"

// Modes
#include "mode.hpp"
#include "normal.hpp"
#include "command.hpp"
#include "search.hpp"

#include "assert.hpp"
#include "config.hpp"
#include "error.hpp"
#include "settings.hpp"

using namespace Main;

Vimpc::Vimpc() :
   currentMode_ (Normal),
   settings_    (Main::Settings::Instance()),
   screen_      (settings_, client_),
   client_      (screen_),
   modeTable_   ()
{
   Ui::Search * const search = new Ui::Search (screen_, client_, settings_);
   modeTable_[Command]       = new Ui::Command(screen_, client_, settings_);
   modeTable_[Normal]        = new Ui::Normal (screen_, client_, settings_, *search);
   modeTable_[Search]        = search;

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

void Vimpc::Run()
{  
   // \todo this is a hack
   static uint32_t updateCount = 0;

   Ui::Command & commandMode = assert_reference(dynamic_cast<Ui::Command *>(modeTable_[Command]));

   bool const configExecutionResult = Config::ExecuteConfigCommands(commandMode);

   if (configExecutionResult == true)
   {
      Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);

      // If we didn't connect to a host from the config file, just connect to the localhost
      if (client_.Connected() == false)
      {
         client_.Connect();
      }

      // If we still have no connection, report an error
      if (client_.Connected() == false)
      {
         Error(1, "Failed to connect to server, please ensure it is running and type :connect <server>");
      }

      screen_.Start();
      mode.Initialise(0);

      bool running = true;

      while (running == true)
      {
         int input = Input();

         if (input != ERR)
         {
            running = Handle(input);
         }
         // \todo do this properly
         // currently if you hold down a scroll times will not update
         else //Should happen every .5 of a second regardless
         {
            if (updateCount > 15)
            {
               updateCount = 0;
               screen_.Update(); //Should happen every 1.5 seconds
            }
            else
            {
               updateCount++;
            }

            // \todo make this more efficient
            Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);
            client_.DisplaySongInformation();
            mode.Refresh();
         }
      }
   }
}


int Vimpc::Input() const
{
   return screen_.WaitForInput();
}

bool Vimpc::Handle(int input)
{
   Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);

   // Input must be handled before mode is changed
   bool const result = mode.Handle(input);

   if (RequiresModeChange(input) == true)
   {
      ChangeMode(input);
   }

   return result;
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

      if (normalMode.CausesModeToStart(input) == true)
      {   
         newMode = Normal;
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
