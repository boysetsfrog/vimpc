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

   command.cpp - tests for command mode code
   */

#include <cppunit/extensions/HelperMacros.h>

#include "algorithm.hpp"
#include "buffers.hpp"
#include "output.hpp"
#include "test.hpp"
#include "vimpc.hpp"

#include "buffer/outputs.hpp"
#include "mode/command.hpp"
#include "window/debug.hpp"
#include "window/error.hpp"

class CommandTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(CommandTester);
   CPPUNIT_TEST(CommandSplit);
   CPPUNIT_TEST(Execution);

   CPPUNIT_TEST(SetCommand);
   CPPUNIT_TEST(TabCommands);
   CPPUNIT_TEST(SetActiveWindowCommands);
   CPPUNIT_TEST(StateCommands);
   CPPUNIT_TEST(OutputCommands);
   CPPUNIT_TEST_SUITE_END();

public:
   CommandTester() :
      settings_(Main::Settings::Instance()),
      commandMode_(*Main::Tester::Instance().Command),
      screen_(*Main::Tester::Instance().Screen),
      client_(*Main::Tester::Instance().Client),
      clientState_(*Main::Tester::Instance().ClientState)
      { }

public:
   void setUp();
   void tearDown();

protected:
   void CommandSplit();
   void Execution();

   void SetCommand();
   void TabCommands();
   void SetActiveWindowCommands();
   void StateCommands();
   void OutputCommands();

private:
   void ActiveWindow(std::string window, Ui::Screen::MainWindow);

private:
   Main::Settings   & settings_;
   Ui::Command      & commandMode_;
   Ui::Screen       & screen_;
   Mpc::Client      & client_;
   Mpc::ClientState & clientState_;
   int32_t            window_;
};

void CommandTester::setUp()
{
   Ui::ErrorWindow::Instance().ClearError();
   window_ = screen_.GetActiveWindow();
}

void CommandTester::tearDown()
{
   screen_.SetActiveAndVisible(window_);
   Ui::ErrorWindow::Instance().ClearError();
}

void CommandTester::CommandSplit()
{
   std::string range, command, arguments;
   commandMode_.SplitCommand("echo", range, command, arguments);
   CPPUNIT_ASSERT((range == "") && (command == "echo") && (arguments == ""));

   commandMode_.SplitCommand("echo test", range, command, arguments);
   CPPUNIT_ASSERT((range == "") && (command == "echo") && (arguments == "test"));

   commandMode_.SplitCommand("1echo testing", range, command, arguments);
   CPPUNIT_ASSERT((range == "1") && (command == "echo") && (arguments == "testing"));

   commandMode_.SplitCommand("1,2echo testing", range, command, arguments);
   CPPUNIT_ASSERT((range == "1,2") && (command == "echo") && (arguments == "testing"));

   commandMode_.SplitCommand("1,2echo testing a longer argument", range, command, arguments);
   CPPUNIT_ASSERT((range == "1,2") && (command == "echo") && (arguments == "testing a longer argument"));

   std::vector<std::string> args = commandMode_.SplitArguments(arguments);
   CPPUNIT_ASSERT(args.size() == 4);
   CPPUNIT_ASSERT((args[0] == "testing") && (args[1] == "a") && (args[2] == "longer") && (args[3] == "argument"));
}

void CommandTester::Execution()
{
   // Ensure that the error command is run and that an error is set
   Ui::ErrorWindow::Instance().ClearError();
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("error 1");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();

   // Test aliasing of commands to another name
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("thisisnotarealcommand");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("alias thisisnotarealcommand sleep 0");
   commandMode_.ExecuteCommand("thisisnotarealcommand");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("unalias thisisnotarealcommand");
   commandMode_.ExecuteCommand("thisisnotarealcommand");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();
}


void CommandTester::SetCommand()
{
   // Test that calling the set command changes a setting
   std::string window = settings_.Get(Setting::Window);
   commandMode_.ExecuteCommand("set " + settings_.Name(Setting::Window) + " playlist");
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == "playlist");
   commandMode_.ExecuteCommand("set " + settings_.Name(Setting::Window) + " test");
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == "test");
   commandMode_.ExecuteCommand("set " + settings_.Name(Setting::Window) + " " + window);
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == window);
}

void CommandTester::TabCommands()
{
   char Buffer[128];

   // Ensure that we change to the first tab
   commandMode_.ExecuteCommand("tabfirst");
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);

   // Ensure that we change to the last tab
   commandMode_.ExecuteCommand("tablast");
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == static_cast<int32_t>(screen_.VisibleWindows() - 1));

   // Ensure that we can move the current tab to position 0 and back
   screen_.SetActiveAndVisible(window_);
   int32_t Index = screen_.GetActiveWindowIndex();
   commandMode_.ExecuteCommand("tabmove 0");
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);

   snprintf(Buffer, 128, "%d", Index);
   commandMode_.ExecuteCommand("tabmove " + std::string(Buffer));
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == Index);

   // Try and rename a tab and ensure the name changes
   std::string name = screen_.GetNameFromWindow(screen_.GetActiveWindow());
   commandMode_.ExecuteCommand("tabrename aridiculoustestname");
   CPPUNIT_ASSERT(screen_.GetNameFromWindow(screen_.GetActiveWindow()) == "aridiculoustestname");
   commandMode_.ExecuteCommand("tabrename " + name);
   CPPUNIT_ASSERT(screen_.GetNameFromWindow(screen_.GetActiveWindow()) == name);

   bool     visible = true;
   int32_t  window  = window_;

   if (window_ >= static_cast<int32_t>(Ui::Screen::MainWindowCount))
   {
      visible = screen_.IsVisible(Ui::Screen::TestConsole);
      screen_.SetActiveAndVisible(Ui::Screen::TestConsole);
      window = Ui::Screen::TestConsole;
   }

   if (screen_.GetActiveWindow() < static_cast<int32_t>(Ui::Screen::MainWindowCount))
   {
      std::string wname = screen_.GetNameFromWindow(screen_.GetActiveWindow());
      screen_.SetActiveAndVisible(window);

      // Test closing the current tab
      CPPUNIT_ASSERT(screen_.IsVisible(window) == true);
      commandMode_.ExecuteCommand("tabclose");
      CPPUNIT_ASSERT(screen_.IsVisible(window) == false);
      screen_.SetActiveAndVisible(window);
      CPPUNIT_ASSERT(screen_.GetActiveWindow() == window);

      // Move the current tab back to the correct position
      snprintf(Buffer, 128, "%d", Index);
      commandMode_.ExecuteCommand("tabmove " + std::string(Buffer));
      CPPUNIT_ASSERT(screen_.IsVisible(window) == true);
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == Index);

      // Test hiding/closing a tab by name
      screen_.SetActiveWindow(0);
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);
      commandMode_.ExecuteCommand("tabhide " + wname);
      CPPUNIT_ASSERT(screen_.IsVisible(window) == false);

      // If we didn't close the active tab we should not
      // change which window is focused
      if (Index != 0)
      {
         CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);
      }

      screen_.SetActiveAndVisible(window);
      CPPUNIT_ASSERT(screen_.GetActiveWindow() == window);

      // Move the tab back to where it started
      snprintf(Buffer, 128, "%d", Index);
      commandMode_.ExecuteCommand("tabmove " + std::string(Buffer));
      CPPUNIT_ASSERT(screen_.IsVisible(window) == true);
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == Index);
   }
   else
   {
      CPPUNIT_ASSERT(false);
   }

   if (visible == false)
   {
      screen_.SetVisible(Ui::Screen::TestConsole, false);
   }
}

void CommandTester::SetActiveWindowCommands()
{
   // Make sure that the correct windows are opened
   ActiveWindow("browse",       Ui::Screen::Browse);
   ActiveWindow("console",      Ui::Screen::Console);
   ActiveWindow("help",         Ui::Screen::Help);
   ActiveWindow("library",      Ui::Screen::Library);
   ActiveWindow("directory",    Ui::Screen::Directory);
   ActiveWindow("playlist",     Ui::Screen::Playlist);
   ActiveWindow("outputs",      Ui::Screen::Outputs);
   ActiveWindow("lists",        Ui::Screen::Lists);
   ActiveWindow("windowselect", Ui::Screen::WindowSelect);
}

void CommandTester::ActiveWindow(std::string window, Ui::Screen::MainWindow main)
{
   // Make sure that the correct windows are opened
   bool visible = screen_.IsVisible(main);
   commandMode_.ExecuteCommand(window);
   CPPUNIT_ASSERT(static_cast<Ui::Screen::MainWindow>(screen_.GetActiveWindow() == main));
   CPPUNIT_ASSERT(screen_.IsVisible(main) == true);
   screen_.SetVisible(main, visible);
}

void CommandTester::StateCommands()
{
   bool    Random    = clientState_.Random();
   bool    Single    = clientState_.Single();
   bool    Consume   = clientState_.Consume();
   bool    Repeat    = clientState_.Repeat();
   int32_t Crossfade = clientState_.Crossfade();
   int32_t Volume    = clientState_.Volume();
   std::string State = clientState_.CurrentState();

   // Test that all the states are toggled
   commandMode_.ExecuteCommand("random");
   Main::Vimpc::WaitForEvent(Event::Random, 5000);
   CPPUNIT_ASSERT(Random != clientState_.Random());

   commandMode_.ExecuteCommand("repeat");
   Main::Vimpc::WaitForEvent(Event::Repeat, 5000);
   CPPUNIT_ASSERT(Repeat != clientState_.Repeat());

   commandMode_.ExecuteCommand("single");
   Main::Vimpc::WaitForEvent(Event::Single, 5000);
   CPPUNIT_ASSERT(Single != clientState_.Single());

   commandMode_.ExecuteCommand("consume");
   Main::Vimpc::WaitForEvent(Event::Consume, 5000);
   CPPUNIT_ASSERT(Consume != clientState_.Consume());

   // Test that all the states are turned on
   commandMode_.ExecuteCommand("random on");
   Main::Vimpc::WaitForEvent(Event::Random, 5000);
   CPPUNIT_ASSERT(clientState_.Random() == true);

   commandMode_.ExecuteCommand("repeat on");
   Main::Vimpc::WaitForEvent(Event::Repeat, 5000);
   CPPUNIT_ASSERT(clientState_.Repeat() == true);

   commandMode_.ExecuteCommand("single on");
   Main::Vimpc::WaitForEvent(Event::Single, 5000);
   CPPUNIT_ASSERT(clientState_.Single() == true);

   commandMode_.ExecuteCommand("consume on");
   Main::Vimpc::WaitForEvent(Event::Consume, 5000);
   CPPUNIT_ASSERT(clientState_.Consume() == true);

   // Test that all the states are turned off
   commandMode_.ExecuteCommand("random off");
   Main::Vimpc::WaitForEvent(Event::Random, 5000);
   CPPUNIT_ASSERT(clientState_.Random() == false);

   commandMode_.ExecuteCommand("repeat off");
   Main::Vimpc::WaitForEvent(Event::Repeat, 5000);
   CPPUNIT_ASSERT(clientState_.Repeat() == false);

   commandMode_.ExecuteCommand("single off");
   Main::Vimpc::WaitForEvent(Event::Single, 5000);
   CPPUNIT_ASSERT(clientState_.Single() == false);

   commandMode_.ExecuteCommand("consume off");
   Main::Vimpc::WaitForEvent(Event::Consume, 5000);
   CPPUNIT_ASSERT(clientState_.Consume() == false);

   // Restore their original values
   client_.SetRandom((Random == true));
   client_.SetRepeat((Repeat == true));
   client_.SetSingle((Single == true));
   client_.SetConsume((Consume == true));

   client_.WaitForCompletion();

   // If we are currently playing, pause before we go
   // messing about with the volume
   if (Algorithm::iequals(State, "playing") == true)
   {
      client_.Pause();
   }

   // \TODO TBD: need to ensure that outputs are enabled
   //       before doing this test
   // Try min, max, mid and invalid volumes
   Ui::ErrorWindow::Instance().ClearError();

   commandMode_.ExecuteCommand("volume 0");
   Main::Vimpc::WaitForEvent(Event::Volume, 5000);
   CPPUNIT_ASSERT(clientState_.Volume() == 0);

   commandMode_.ExecuteCommand("volume 100");
   Main::Vimpc::WaitForEvent(Event::Volume, 5000);
   CPPUNIT_ASSERT(clientState_.Volume() == 100);

   commandMode_.ExecuteCommand("volume 50");
   Main::Vimpc::WaitForEvent(Event::Volume, 5000);
   CPPUNIT_ASSERT(clientState_.Volume() == 50);
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);

   commandMode_.ExecuteCommand("volume 500");
   Main::Vimpc::WaitForEvent(Event::Volume, 5000);
   CPPUNIT_ASSERT(clientState_.Volume() == 50);
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();

   client_.SetVolume(Volume);
   Main::Vimpc::WaitForEvent(Event::Volume, 5000);

   // Set mpd to whatever state it was in before
   if (Algorithm::iequals(State, "playing") == true)
   {
      client_.Pause();
   }
}


void CommandTester::OutputCommands()
{
   char Buffer[128];

   bool visible = screen_.IsVisible(Ui::Screen::Outputs);
   screen_.SetActiveAndVisible(Ui::Screen::Outputs);

   int32_t outputCount = Main::Outputs().Size();
   int32_t currentLine = screen_.ActiveWindow().CurrentLine();

   std::map<int32_t, bool> outputs;

   for (int i = 0; i < outputCount; ++i)
   {
      outputs[i] = Main::Outputs().Get(i)->Enabled();

      // Ensure that the selected outputs are enabled/disabled
      screen_.ScrollTo(i);
      commandMode_.ExecuteCommand("enable");
      Main::Vimpc::WaitForEvent(Event::OutputEnabled, 5000);
      CPPUNIT_ASSERT(Main::Outputs().Get(i)->Enabled() == true);

      commandMode_.ExecuteCommand("disable");
      Main::Vimpc::WaitForEvent(Event::OutputDisabled, 5000);
      CPPUNIT_ASSERT(Main::Outputs().Get(i)->Enabled() == false);

      // Ensure that enable using the id works
      screen_.ScrollTo(outputCount);
      snprintf(Buffer, 128, "enable %d", i);
      commandMode_.ExecuteCommand(Buffer);
      Main::Vimpc::WaitForEvent(Event::OutputEnabled, 5000);
      CPPUNIT_ASSERT(Main::Outputs().Get(i)->Enabled() == true);

      // Ensure that disable using the id works
      screen_.ScrollTo(outputCount);
      snprintf(Buffer, 128, "disable %d", i);
      commandMode_.ExecuteCommand(Buffer);
      Main::Vimpc::WaitForEvent(Event::OutputDisabled, 5000);
      CPPUNIT_ASSERT(Main::Outputs().Get(i)->Enabled() == false);

      // Ensure that a line based command enables the correct output
      screen_.ScrollTo(outputCount);
      snprintf(Buffer, 128, "%denable", i+1);
      commandMode_.ExecuteCommand(Buffer);
      Main::Vimpc::WaitForEvent(Event::OutputEnabled, 5000);
      CPPUNIT_ASSERT(Main::Outputs().Get(i)->Enabled() == true);

      // Ensure that a line based command disables the correct output
      screen_.ScrollTo(outputCount);
      snprintf(Buffer, 128, "%ddisable", i+1);
      commandMode_.ExecuteCommand(Buffer);
      Main::Vimpc::WaitForEvent(Event::OutputDisabled, 5000);
      CPPUNIT_ASSERT(Main::Outputs().Get(i)->Enabled() == false);
   }

   // Ensure that range based enables and disables work
   snprintf(Buffer, 128, "%d,%denable", 1, outputCount);
   commandMode_.ExecuteCommand(Buffer);

   for (int i = 0; i < outputCount; ++i)
   {
      Main::Vimpc::WaitForEvent(Event::OutputEnabled, 1000);
      CPPUNIT_ASSERT(Main::Outputs().Get(i)->Enabled() == true);
   }

   snprintf(Buffer, 128, "%d,%ddisable", 1, outputCount);
   commandMode_.ExecuteCommand(Buffer);

   for (int i = 0; i < outputCount; ++i)
   {
      Main::Vimpc::WaitForEvent(Event::OutputDisabled, 1000);
      CPPUNIT_ASSERT(Main::Outputs().Get(i)->Enabled() == false);
   }

   // \TODO TBD: test visual selection enable/disable

   // Restore outputs to their initial state
   for (int i = 0; i < outputCount; ++i)
   {
      if (outputs[i] == true)
      {
         client_.EnableOutput(Main::Outputs().Get(i));
      }
      else
      {
         client_.DisableOutput(Main::Outputs().Get(i));
      }
   }

   client_.WaitForCompletion();
   screen_.ScrollTo(currentLine);
   screen_.SetVisible(Ui::Screen::Outputs, visible);
}

CPPUNIT_TEST_SUITE_REGISTRATION(CommandTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CommandTester, "command");
