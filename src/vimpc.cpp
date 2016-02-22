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

#include "compiler.hpp"

#include "mode/mode.hpp"
#include "mode/normal.hpp"
#include "mode/command.hpp"
#include "mode/search.hpp"

#include "assert.hpp"
#include "buffers.hpp"
#include "config.hpp"
#include "events.hpp"
#include "settings.hpp"
#include "song.hpp"
#include "test.hpp"

#include "buffer/directory.hpp"
#include "buffer/outputs.hpp"
#include "buffer/playlist.hpp"
#include "window/error.hpp"
#include "window/songwindow.hpp"

#include <list>
#include <unistd.h>

using namespace Main;

typedef std::pair<int32_t, EventData>  EventPair;

static std::list<EventPair>            Queue;
static std::map<int, std::vector<FUNCTION<void(EventData const &)> > > Handler;

static Mutex               EventMutex;
static Mutex               QueueMutex;
static ConditionVariable   Condition;

static std::map<int, std::list<ConditionVariable *> > WaitConditions;

bool Vimpc::Running = true;

// \todo the coupling and requirements on the way everything needs to be constructed is awful
// this really needs to be fixed and the coupling removed
Vimpc::Vimpc() :
   currentMode_      (Normal),
   settings_         (Main::Settings::Instance()),
   search_           (*(new Ui::Search (screen_, client_, settings_))),
   screen_           (settings_, client_, clientState_, search_),
   client_           (this, settings_, Main::AllLists(), screen_),
   clientState_      (this, settings_, screen_),
   modeTable_        (),
   normalMode_       (*(new Ui::Normal (this, screen_, client_, clientState_, settings_, search_))),
   commandMode_      (*(new Ui::Command(this, screen_, client_, clientState_, settings_, search_, normalMode_))),
   userEvents_       (true),
   requireRepaint_   (false)
{

   modeTable_[Command] = &commandMode_;
   modeTable_[Normal]  = &normalMode_;
   modeTable_[Search]  = &search_;

   ENSURE(modeTable_.size()     == ModeCount);
   ENSURE(ModesAreInitialised() == true);

   Vimpc::EventHandler(Event::Repaint, [this] (EventData const & Data) { SetRepaint(true); });

   Vimpc::EventHandler(Event::Autoscroll, [this] (EventData const & Data)
   {
      normalMode_.HandleAutoScroll();
   });


   // When we change the album artist we need to repopulate the print functions in the song
   // this is an optimisation, if you check the setting for every song print to determine
   // whether to use the albumartist or the artist it is a huge overhead, so we don't
   // we also need to clear the format cache, this occurs in a library callback
   settings_.RegisterCallback(Setting::AlbumArtist, [this] (bool Value)
   {
      Mpc::Song::RepopulateSongFunctions();
   });

#ifdef TEST_ENABLED
   Main::Tester::Instance().Vimpc   = this;
   Main::Tester::Instance().Screen  = &screen_;
   Main::Tester::Instance().Command = &commandMode_;
   Main::Tester::Instance().Client  = &client_;
   Main::Tester::Instance().ClientState = &clientState_;
#endif
}

Vimpc::~Vimpc()
{
   for (auto mode : modeTable_)
   {
      delete (mode.second);
      mode.second = NULL;
   }
}

void Vimpc::Run(std::string hostname, uint16_t port)
{
   int input = ERR;

   // Keyboard input event handler
   Vimpc::EventHandler(Event::Input, [&input] (EventData const & Data)
   {
      input = Data.input;
   });

   // Refresh the mode after a status update
   Vimpc::EventHandler(Event::StatusUpdate, [this] (EventData const & Data)
   {
      if (screen_.PagerIsVisible() == false)
      {
         Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);
         mode.Refresh();
      }
   });

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
      if (commandMode_.ConnectionAttempt() == false)
      {
         client_.Connect(hostname, port);
      }

      screen_.Update();
      commandMode_.SetQueueCommands(false);

      // The main loop
      while (Running == true)
      {
         screen_.UpdateErrorDisplay();

         {
            UniqueLock<Mutex> Lock(QueueMutex);

            if ((Queue.empty() == false) ||
               (ConditionWait(Condition, Lock, 100) != false))
            {
               if (Queue.empty() == false)
               {
                  EventPair const Event = Queue.front();
                  Queue.pop_front();
                  Lock.unlock();

                  if ((userEvents_ == false) &&
                     (Event.second.user == true))
                  {
                     Debug("Discarding user event");
                     continue;
                  }

                  for (auto func : Handler[Event.first])
                  {
                     func(Event.second);
                  }

                  EventMutex.lock();

                  for (auto cond : WaitConditions[Event.first])
                  {
                     cond->notify_all();
                  }

                  EventMutex.unlock();

                  Debug("Event triggered: " + EventStrings::Default[Event.first]);
               }
            }
         }

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

         bool const Resize = screen_.Resize();

         QueueMutex.lock();

         if (((input != ERR) || (Resize == true)) || (requireRepaint_ == true))
         {
            QueueMutex.unlock();
            Repaint();
         }
         else
         {
            QueueMutex.unlock();
         }

         input = ERR;
      }
   }
}

void Vimpc::SetRepaint(bool requireRepaint)
{
   requireRepaint_ = requireRepaint;
}

void Vimpc::Repaint()
{
   requireRepaint_ = false;

   if (Running)
   {
      screen_.Update();
      clientState_.DisplaySongInformation();

      if (screen_.PagerIsVisible() == false)
      {
         Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);
         mode.Refresh();
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
      for (auto nmode : modeTable_)
      {
         if (nmode.first != Normal)
         {
            Ui::Mode const & nextMode = assert_reference(nmode.second);

            if (nextMode.CausesModeToStart(input) == true)
            {
               newMode = nmode.first;
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

void Vimpc::HandleUserEvents(bool Enabled)
{
   userEvents_ = Enabled;
}

/* static */ void Vimpc::SetRunning(bool isRunning)
{
   Running = isRunning;
}

/* static */ void Vimpc::CreateEvent(int Event, EventData const & Data)
{
   UniqueLock<Mutex> Lock(QueueMutex);
   Queue.push_back(std::make_pair(Event, Data));
   Condition.notify_all();
}

/* static */ void Vimpc::EventHandler(int Event, FUNCTION<void(EventData const &)> func)
{
   Handler[Event].push_back(func);
}

/* static */ bool Vimpc::WaitForEvent(int Event, int TimeoutMs)
{
   UniqueLock<Mutex> EventLock(EventMutex);
   ConditionVariable * WaitCondition = new ConditionVariable();

   WaitConditions[Event].push_back(WaitCondition);

   bool const Result = (ConditionWait(*WaitCondition, EventLock, TimeoutMs));

   WaitConditions[Event].remove(WaitCondition);
   delete WaitCondition;
   return Result;
}

int Vimpc::Input() const
{
   if (currentMode_ == Normal)
   {
      return screen_.WaitForInput(250, !normalMode_.WaitingForMoreInput());
   }

   return screen_.WaitForInput(250);
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


bool Vimpc::HandleMouse()
{
   return screen_.HandleMouseEvent();
}

bool Vimpc::ModesAreInitialised()
{
   bool result = true;

   for (auto mode : modeTable_)
   {
      result = (mode.second == NULL) ? false : result;
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
