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

#include "test.hpp"
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
   CPPUNIT_TEST_SUITE_END();

public:
   CommandTester() :
      settings_(Main::Settings::Instance()),
      commandMode_(*Main::Tester::Instance().Command),
      screen_(*Main::Tester::Instance().Screen) { }

public:
   void setUp();
   void tearDown();

protected:
   void CommandSplit();
   void Execution();

   void SetCommand();
   void TabCommands();
   void SetActiveWindowCommands();

private:
   Main::Settings & settings_;
   Ui::Command    & commandMode_;
   Ui::Screen     & screen_;
   int32_t          window_;
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
   commandMode_.ExecuteCommand("set " + settings_.Name(Setting::Window) + " test");
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == "test");
   commandMode_.ExecuteCommand("set " + settings_.Name(Setting::Window) + " " + window);
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == window);
}

void CommandTester::TabCommands()
{
   char Buffer[128];

   commandMode_.ExecuteCommand("tabfirst");
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);

   commandMode_.ExecuteCommand("tablast");
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == (screen_.VisibleWindows() - 1));

   screen_.SetActiveAndVisible(window_);
   int32_t Index = screen_.GetActiveWindowIndex();
   commandMode_.ExecuteCommand("tabmove 0");
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);

   snprintf(Buffer, 128, "%d", Index);
   commandMode_.ExecuteCommand("tabmove " + std::string(Buffer));
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == Index);

   std::string name = screen_.GetNameFromWindow(screen_.GetActiveWindow());
   commandMode_.ExecuteCommand("tabrename aridiculoustestname");
   CPPUNIT_ASSERT(screen_.GetNameFromWindow(screen_.GetActiveWindow()) == "aridiculoustestname");
   commandMode_.ExecuteCommand("tabrename " + name);
   CPPUNIT_ASSERT(screen_.GetNameFromWindow(screen_.GetActiveWindow()) == name);

   if (window_ < (int32_t) Ui::Screen::MainWindowCount)
   {
      screen_.SetActiveAndVisible(window_);

      CPPUNIT_ASSERT(screen_.IsVisible(window_) == true);
      commandMode_.ExecuteCommand("tabclose");
      CPPUNIT_ASSERT(screen_.IsVisible(window_) == false);

      screen_.SetActiveAndVisible(window_);
      CPPUNIT_ASSERT(screen_.GetActiveWindow() == window_);

      snprintf(Buffer, 128, "%d", Index);
      commandMode_.ExecuteCommand("tabmove " + std::string(Buffer));
      CPPUNIT_ASSERT(screen_.IsVisible(window_) == true);
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == Index);

      screen_.SetActiveWindow(0);
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);
      commandMode_.ExecuteCommand("tabhide " + name);
      CPPUNIT_ASSERT(screen_.IsVisible(window_) == false);

      if (Index != 0)
      {
         CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);
      }

      screen_.SetActiveAndVisible(window_);
      CPPUNIT_ASSERT(screen_.GetActiveWindow() == window_);

      snprintf(Buffer, 128, "%d", Index);
      commandMode_.ExecuteCommand("tabmove " + std::string(Buffer));
      CPPUNIT_ASSERT(screen_.IsVisible(window_) == true);
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == Index);
   }
}

void CommandTester::SetActiveWindowCommands()
{
   commandMode_.ExecuteCommand("browse");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Browse);

   commandMode_.ExecuteCommand("console");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Console);

   commandMode_.ExecuteCommand("help");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Help);

   commandMode_.ExecuteCommand("library");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Library);

   commandMode_.ExecuteCommand("directory");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Directory);

   commandMode_.ExecuteCommand("playlist");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Playlist);

   commandMode_.ExecuteCommand("outputs");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Outputs);

   commandMode_.ExecuteCommand("lists");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Lists);

   commandMode_.ExecuteCommand("windowselect");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::WindowSelect);
}

CPPUNIT_TEST_SUITE_REGISTRATION(CommandTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CommandTester, "command");
