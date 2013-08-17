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

   algorithms.cpp - tests for algorithms code
   */

#include <cppunit/extensions/HelperMacros.h>

#include "screen.hpp"
#include "test.hpp"

class ScreenTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(ScreenTester);
   CPPUNIT_TEST(TestChangeWindow);
   CPPUNIT_TEST_SUITE_END();

public:
   ScreenTester() 
      : screen_(*Main::Tester::Instance().Screen) { }

public:
   void setUp();
   void tearDown();

protected:
   void TestChangeWindow();

private:
   Ui::Screen & screen_;
   int32_t      window_;
};

void ScreenTester::setUp()
{
   window_ = screen_.GetActiveWindow();
}

void ScreenTester::tearDown()
{
   screen_.SetActiveAndVisible(window_);
}

// Test that cycles through all the windows
void ScreenTester::TestChangeWindow()
{
   int32_t WindowCount = screen_.VisibleWindows();
   int32_t StartWindow = screen_.GetActiveWindow();
   int32_t PreviousWindow = screen_.GetActiveWindow();

   for (uint32_t i = 0; i < WindowCount; ++i)
   {
      PreviousWindow = screen_.GetActiveWindow();
      screen_.SetActiveWindow(i);

      // Ensure that it is the correct window
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == i);
      CPPUNIT_ASSERT(screen_.IsVisible(screen_.GetActiveWindow()) == true);

      // Ensure that previous is set correctly
      CPPUNIT_ASSERT(screen_.GetPreviousWindow() == PreviousWindow);
   }

   for (int i = 0; i < WindowCount; ++i)
   {
      PreviousWindow = screen_.GetActiveWindow();
      screen_.SetActiveWindow(Ui::Screen::Next);
      CPPUNIT_ASSERT(screen_.IsVisible(screen_.GetActiveWindow()) == true);

      // Ensure that previous is set correctly
      CPPUNIT_ASSERT(screen_.GetPreviousWindow() == PreviousWindow);
   }  

   // Test that cycling through all windows returns to start 
   CPPUNIT_ASSERT(screen_.GetActiveWindow() == StartWindow);

   for (int i = 0; i < WindowCount; ++i)
   {
      PreviousWindow = screen_.GetActiveWindow();
      screen_.SetActiveWindow(Ui::Screen::Previous);
      CPPUNIT_ASSERT(screen_.IsVisible(screen_.GetActiveWindow()) == true);

      // Ensure that previous is set correctly
      CPPUNIT_ASSERT(screen_.GetPreviousWindow() == PreviousWindow);
   }  

   // Test that cycling through all windows returns to start 
   CPPUNIT_ASSERT(screen_.GetActiveWindow() == StartWindow);

   // Check that going to absolute type works
   screen_.SetActiveWindowType((Ui::Screen::MainWindow) StartWindow);
   CPPUNIT_ASSERT(screen_.GetActiveWindow() == StartWindow);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScreenTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ScreenTester, "screen");
