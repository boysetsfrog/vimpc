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

   window.cpp - tests for various window classes
*/

#include <cppunit/extensions/HelperMacros.h>

#include "screen.hpp"
#include "test.hpp"
#include "window/debug.hpp"
#include "window/scrollwindow.hpp"

class WindowTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(WindowTester);
   CPPUNIT_TEST(Test);

   CPPUNIT_TEST_SUITE_END();

public:
   WindowTester()
      : screen_(*Main::Tester::Instance().Screen) { }

public:
   void setUp();
   void tearDown();

protected:
   void Test();

private:
   Ui::Screen & screen_;
   int32_t      window_;
};

void WindowTester::setUp()
{
   window_ = screen_.GetActiveWindow();
}

void WindowTester::tearDown()
{
   screen_.SetActiveAndVisible(window_);
}

void WindowTester::Test()
{
}

CPPUNIT_TEST_SUITE_REGISTRATION(WindowTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(WindowTester, "window");
