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
   CPPUNIT_TEST_SUITE_END();

public:
   CommandTester() :
      settings_(Main::Settings::Instance()),
      commandMode_(*Main::Tester::Instance().Command) { }

public:
   void setUp();
   void tearDown();

protected:
   void CommandSplit();
   void Execution();

private:
   Main::Settings & settings_; 
   Ui::Command    & commandMode_;
};

void CommandTester::setUp()
{
   Ui::ErrorWindow::Instance().ClearError();
}

void CommandTester::tearDown()
{
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
   Ui::ErrorWindow::Instance().ClearError();
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("error 1");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
}

CPPUNIT_TEST_SUITE_REGISTRATION(CommandTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CommandTester, "command");
