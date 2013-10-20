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
#include "events.hpp"
#include "settings.hpp"
#include "song.hpp"
#include "songsorter.hpp"
#include "test.hpp"

#include "buffer/directory.hpp"
#include "buffer/outputs.hpp"
#include "buffer/playlist.hpp"
#include "window/error.hpp"
#include "window/songwindow.hpp"

#include <atomic>
#include <list>
#include <unistd.h>
#include <condition_variable>

using namespace Main;

typedef std::pair<int32_t, EventData>  EventPair;
static std::list<EventPair>            Queue;
static std::mutex                      QueueMutex;
static std::condition_variable         Condition;
static std::map<int, std::vector<std::function<void(EventData const &)> > > Handler;

static std::map<int, std::list<std::condition_variable *> > WaitConditions;
static std::mutex EventMutex;

bool Vimpc::Running = true;

// \todo the coupling and requirements on the way everything needs to be constructed is awful
// this really needs to be fixed and the coupling removed
Vimpc::Vimpc() :
   currentMode_      (Normal),
   settings_         (Main::Settings::Instance()),
   search_           (*(new Ui::Search (screen_, client_, settings_))),
   screen_           (settings_, client_, clientState_, search_),
   client_           (this, settings_, screen_),
   clientState_      (this, settings_, screen_),
   modeTable_        (),
   normalMode_       (*(new Ui::Normal (this, screen_, client_, clientState_, settings_, search_))),
   commandMode_      (*(new Ui::Command(this, screen_, client_, clientState_, settings_, search_, normalMode_))),
   clientUpdate_     (false),
   clientQueueUpdate_(false),
   userEvents_       (true)
{

   modeTable_[Command] = &commandMode_;
   modeTable_[Normal]  = &normalMode_;
   modeTable_[Search]  = &search_;

   ENSURE(modeTable_.size()     == ModeCount);
   ENSURE(ModesAreInitialised() == true);

   //
   Vimpc::EventHandler(Event::Connected, [this] (EventData const & Data)
   {
      this->clientUpdate_ = true;
   });

   Vimpc::EventHandler(Event::StatusUpdate, [this] (EventData const & Data)
   {
      this->clientUpdate_ = true;
   });

   Vimpc::EventHandler(Event::QueueUpdate, [this] (EventData const & Data)
   {
      this->clientQueueUpdate_ = true;
   });

   Vimpc::EventHandler(Event::AllMetaDataReady, [this] (EventData const & Data)
   {
      this->clientQueueUpdate_ = true;
   });

   Vimpc::EventHandler(Event::ClearDatabase, [this] (EventData const & Data)
   {
      Main::Playlist().Clear();
      Main::Directory().Clear(true);
      Main::Library().Clear();

      Main::AllLists().Clear();
      Main::MpdLists().Clear();
      Main::FileLists().Clear();

      Main::PlaylistPasteBuffer().Clear();
   });

   Vimpc::EventHandler(Event::DatabaseSong, [this] (EventData const & Data)
   {
      Main::Library().Add(Data.song);
      Main::Directory().Add(Data.song);
   });

   Vimpc::EventHandler(Event::DatabasePath, [this] (EventData const & Data)
   {
      Main::Directory().Add(Data.uri);
   });

   Vimpc::EventHandler(Event::DatabaseListFile, [this] (EventData const & Data)
   {
      Mpc::List list(Data.uri, Data.name);
      Main::Directory().AddPlaylist(list);
      Main::FileLists().Add(list);
      Main::AllLists().Add(list);
   });

   Vimpc::EventHandler(Event::DatabaseList, [this] (EventData const & Data)
   {
      Mpc::List const list(Data.name);
      Main::MpdLists().Add(list);
      Main::AllLists().Add(list);
   });

   Vimpc::EventHandler(Event::CommandListSend, [this] (EventData const & Data)
   {
      Debug("Command list send bit");
      this->clientQueueUpdate_ = true;
   });

   Vimpc::EventHandler(Event::SearchResults, [this] (EventData const & Data)
   {
      Ui::SongWindow * const window = screen_.CreateSongWindow(Data.name);

      for (auto uri : Data.uris)
      {
         Mpc::Song * song = Main::Library().Song(uri);

         if (song != NULL)
         {
            window->Buffer().Add(song);
         }
      }

      Ui::SongSorter const sorter(settings_.Get(::Setting::Sort));
      window->Buffer().Sort(sorter);
      screen_.SetActiveAndVisible(screen_.GetWindowFromName(window->Name()));
   });

   Vimpc::EventHandler(Event::PlaylistContents, [this] (EventData const & Data)
   {
      Ui::SongWindow * const window = screen_.CreateSongWindow("P:" + Data.name);

      for (auto uri : Data.uris)
      {
         Mpc::Song * song = Main::Library().Song(uri);

         if (song != NULL)
         {
            window->Buffer().Add(song);
         }
      }

      screen_.SetActiveAndVisible(screen_.GetWindowFromName(window->Name()));
   });

   Vimpc::EventHandler(Event::PlaylistContentsForRemove, [this] (EventData const & Data)
   {
      Ui::SongWindow * const window = screen_.CreateSongWindow("P:" + Data.name);

      Mpc::CommandList list(client_);

      for (auto uri : Data.uris)
      {
         Mpc::Song * song = Main::Library().Song(uri);

         int const PlaylistIndex = Main::Playlist().Index(song);

         if (PlaylistIndex >= 0)
         {
            client_.Delete(PlaylistIndex);
         }
      }
   });

   Vimpc::EventHandler(Event::Output, [this] (EventData const & Data)
   {
      Main::Outputs().Add(Data.output);
      clientQueueUpdate_ = true;
   });

   Vimpc::EventHandler(Event::OutputEnabled,  [this] (EventData const & Data) 
   { 
      clientQueueUpdate_ = true;
   });

   Vimpc::EventHandler(Event::OutputDisabled, [this] (EventData const & Data)
   {
      clientQueueUpdate_ = true;
   });

   Vimpc::EventHandler(Event::NewPlaylist, [] (EventData const & Data)
   {
      if (Main::AllLists().Index(Mpc::List(Data.name)) == -1)
      {
         Main::AllLists().Add(Data.name);
         Main::AllLists().Sort();
         Main::MpdLists().Add(Data.name);
         Main::MpdLists().Sort();
      }
   });

   //
   Vimpc::EventHandler(Event::PlaylistAdd, [] (EventData const & Data)
   {
      Mpc::Song * song = Main::Library().Song(Data.uri);

      if (song == NULL)
      {
         song = new Mpc::Song();
         song->SetURI(Data.uri.c_str());
      }

      if (Data.pos1 == -1)
      {
         Main::Playlist().Add(song);
      }
      else
      {
         Main::Playlist().Add(song, Data.pos1);
      }
   });

   Vimpc::EventHandler(Event::PlaylistQueueReplace, [] (EventData const & Data)
   {
      Debug("Playlist Queue Replace Event");

      for (auto pair : Data.posuri)
      {
         Mpc::Song * song = Main::Library().Song(pair.second);

         if (song == NULL)
         {
            song = new Mpc::Song();
            song->SetURI(pair.second.c_str());
         }

         Main::Playlist().Replace(pair.first, song);
      }

      Main::Playlist().Crop(Data.count);
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
            std::unique_lock<std::mutex> Lock(QueueMutex);

            if ((Queue.empty() == false) ||
               (Condition.wait_for(Lock, std::chrono::milliseconds(100)) != std::cv_status::timeout))
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

               for (auto cond : WaitConditions[Event.first])
               {
                  cond->notify_all();
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

         // \TODO client needs to tell this to force an update somehow
         if ((input != ERR) || (Resize == true) ||
             (clientUpdate_ == true) || (clientQueueUpdate_ == true))
         {
            clientUpdate_ = false;

            if ((input != ERR) || (Resize == true) ||
                (clientQueueUpdate_ == true))
            {
               Debug("Doing the screen update");
               clientQueueUpdate_ = false;
               screen_.Update();
               clientState_.DisplaySongInformation();
            }

            if (screen_.PagerIsVisible() == false)
            {
               Ui::Mode & mode = assert_reference(modeTable_[currentMode_]);
               mode.Refresh();
            }
         }

         input = ERR;
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
   std::unique_lock<std::mutex> Lock(QueueMutex);
   Queue.push_back(std::make_pair(Event, Data));
   Condition.notify_all();
}

/* static */ void Vimpc::EventHandler(int Event, std::function<void(EventData const &)> func)
{
   Handler[Event].push_back(func);
}

/* static */ bool Vimpc::WaitForEvent(int Event, int TimeoutMs)
{
   std::unique_lock<std::mutex> EventLock(EventMutex);

   std::condition_variable * WaitCondition = new std::condition_variable();
   WaitConditions[Event].push_back(WaitCondition);

   bool const Result = (WaitCondition->wait_for(EventLock, std::chrono::milliseconds(TimeoutMs)) != std::cv_status::timeout);

   WaitConditions[Event].remove(WaitCondition);
   delete WaitCondition;
   EventLock.unlock();
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
