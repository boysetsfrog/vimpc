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
#include "test.hpp"
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
   modeTable_   (),
   normalMode_  (*(new Ui::Normal (this, screen_, client_, settings_, search_))),
   commandMode_ (*(new Ui::Command(this, screen_, client_, settings_, search_, normalMode_))),
   clientUpdate_(false)
{

   modeTable_[Command] = &commandMode_;
   modeTable_[Normal]  = &normalMode_;
   modeTable_[Search]  = &search_;

   ENSURE(modeTable_.size()     == ModeCount);
   ENSURE(ModesAreInitialised() == true);

#ifdef TEST_ENABLED
   Main::Tester::Instance().Screen  = &screen_;
   Main::Tester::Instance().Command = &commandMode_;
   Main::Tester::Instance().Client  = &client_;
#endif
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
   // Set up the display
   {
      Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);
      mode.Initialise(0);
   }

   SetSkipConfigConnects((hostname != "") || (port != 0));

   // Parse the config file
   commandMode_.SetQueueCommands(true);
   bool const configExecutionResult = Config::ExecuteConfigCommands(commandMode_);

   SetSkipConfigConnects(false);

   screen_.Start();

   if (configExecutionResult == true)
   {
      // If we didn't connect to a host from the config file, just connect to the default
      if (client_.Connected() == false)
      {
         client_.Connect(hostname, port);
      }

      client_.DisplaySongInformation();
      screen_.Update();
      commandMode_.SetQueueCommands(false);

      struct timeval start, end;
      gettimeofday(&start, NULL);

      // The main loop
      while (Running == true)
      {
         static long updateTime = 0;

			screen_.UpdateErrorDisplay();

         int input = Input();

         if (input != ERR)
			{
				screen_.ClearErrorDisplay();

				if ((screen_.PagerIsVisible() == true)
#ifdef HAVE_MOUSE_SUPPORT
					&& (input != KEY_MOUSE)
#endif
            )
				{
					if (screen_.PagerIsFinished() == true)
					{
						screen_.HidePagerWindow();
					}
					else
					{
						screen_.PagerWindowNext();
					}
				}
				else
				{
					Handle(input);
				}
			}

         gettimeofday(&end,   NULL);

         long const seconds  = end.tv_sec  - start.tv_sec;
         long const useconds = end.tv_usec - start.tv_usec;
         long const mtime    = (seconds * 1000 + (useconds/1000.0)) + 0.5;

         updateTime += mtime;
         client_.IncrementTime(mtime);

         if ((input == ERR) && ((client_.TimeSinceUpdate() > 900) &&
             (settings_.Get(::Setting::Polling) == true)))
         {
            client_.UpdateStatus();
            clientUpdate_ = true;
         }

         gettimeofday(&start, NULL);

         // \TODO client needs to tell this to force an update somehow
         if ((input != ERR) || (screen_.Resize() == true) || (clientUpdate_ == true) ||
             ((updateTime >= 250) && (input == ERR)))
         {
            clientUpdate_ = false;
            updateTime   = 0;

            client_.DisplaySongInformation();
            client_.UpdateDisplay();
            screen_.Update();

            if (screen_.PagerIsVisible() == false)
            {
               Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);
               mode.Refresh();
            }
         }
      }
   }
}

Ui::Mode & Vimpc::CurrentMode()
{
   return assert_reference(modeTable_[currentMode_]);
}

bool Vimpc::RequiresModeChange(ModeName mode, int input) const
{
   return (mode != ModeAfterInput(mode, input));
}

Vimpc::ModeName Vimpc::ModeAfterInput(ModeName mode, int input) const
{
   ModeName newMode = mode;

   // Check if we are returning to normal mode
   if (mode != Normal)
   {
      ModeTable::const_iterator it = modeTable_.find(mode);
      Ui::Mode const & currentMode = assert_reference(it->second);

      if (currentMode.CausesModeToEnd(input) == true)
      {
         newMode = Normal;
      }
   }
   // Must be in normal mode to be able to enter any other mode
   else
   {
      for (ModeTable::const_iterator it = modeTable_.begin(); (it != modeTable_.end()); ++it)
      {
         if (it->first != Normal)
         {
            Ui::Mode const & nextMode = assert_reference(it->second);

            if (nextMode.CausesModeToStart(input) == true)
            {
               newMode = it->first;
            }
         }
      }
   }

   return newMode;
}

void Vimpc::ChangeMode(char input, std::string initial)
{
   ChangeMode(input);

   Ui::InputMode * mode = dynamic_cast<Ui::InputMode *> (modeTable_.find(currentMode_)->second);

   if (mode != NULL)
   {
      bool finished = mode->SetInputString(initial);

      if (finished == true)
      {
         // The string input complete, change the mode as if
         // a return key was pressed
         ChangeMode('\n');
         normalMode_.Refresh();
      }
      else
      {
         mode->Refresh();
      }
   }
}


/* static */ void Vimpc::SetRunning(bool isRunning)
{
   Running = isRunning;
}


int Vimpc::Input() const
{
   if (currentMode_ == Normal)
   {
      return screen_.WaitForInput(200, !normalMode_.WaitingForMoreInput());
   }

   return screen_.WaitForInput(200);
}

void Vimpc::Handle(int input)
{
   Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);

#ifdef HAVE_MOUSE_SUPPORT
   if ((input == KEY_MOUSE) && (settings_.Get(::Setting::Mouse) == true))
   {
      mode = assert_reference(modeTable_[Vimpc::Normal]);
      bool const Finished = HandleMouse();

      if (Finished)
      {
         return;
      }
   }

#endif

   // Only change modes explicitly when Handle didn't
   ModeName const currentMode = currentMode_;

   // Input must be handled before mode is changed
   mode.Handle(input);

   if ((currentMode == currentMode_) &&
       (RequiresModeChange(currentMode_, input) == true))
   {
      ChangeMode(input);
      mode.Refresh();
   }
}

void Vimpc::OnConnected()
{
   // \TODO ideally this should be run out of the main thread
   //       not in this callback might need to set a flag to indicate
   //       that it needs to be run
   commandMode_.ExecuteQueuedCommands();

   clientUpdate_ = true;
}

bool Vimpc::HandleMouse()
{
   return screen_.HandleMouseEvent();
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


void Vimpc::ChangeMode(int input)
{
   ModeName const oldModeName = currentMode_;
   ModeName const newModeName = ModeAfterInput(currentMode_, input);

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
