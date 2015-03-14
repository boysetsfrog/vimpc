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
#include "window/debug.hpp"
#include "window/scrollwindow.hpp"

class ScreenTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(ScreenTester);
   CPPUNIT_TEST(TestChangeWindow);

   CPPUNIT_TEST(TestAlign);
   CPPUNIT_TEST(TestAlignTo);
   CPPUNIT_TEST(TestSelect);

   CPPUNIT_TEST_SUITE_END();

public:
   ScreenTester()
      : screen_(*Main::Tester::Instance().Screen) { }

public:
   void setUp();
   void tearDown();

protected:
   void TestChangeWindow();
   void TestAlign();
   void TestAlignTo();
   void TestSelect();

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

   // Cycle through every window using the index in the tab bar
   for (int i = 0; i < WindowCount; ++i)
   {
      PreviousWindow = screen_.GetActiveWindow();
      screen_.SetActiveWindow(i);

      // Ensure that it is the correct window
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == i);
      CPPUNIT_ASSERT(screen_.IsVisible(screen_.GetActiveWindow()) == true);

      // Ensure that previous is set correctly
      CPPUNIT_ASSERT(screen_.GetPreviousWindow() == PreviousWindow);
   }

   screen_.SetActiveAndVisible(StartWindow);

   // Cycle through all windows using :tabnext
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

   // Cycle through all windows using :tabprevious
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

   // Check that going to absolute window works
   screen_.SetActiveAndVisible(StartWindow);
   CPPUNIT_ASSERT(screen_.GetActiveWindow() == StartWindow);
}

void ScreenTester::TestAlign() // ^E, ^Y
{
   screen_.SetActiveAndVisible(Ui::Screen::MainWindow::Browse);

   int32_t rows = screen_.MaxRows();
   int32_t bufferSize = screen_.ActiveWindow().BufferSize();

   screen_.ScrollTo(0);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == 0);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == rows);

   screen_.Align(Ui::Screen::Direction::Down, 3);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == 3);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == rows + 3);

   screen_.Align(Ui::Screen::Direction::Up, 2);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1== rows + 1);

   screen_.Align(Ui::Screen::Direction::Up, 2);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == 0);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == rows);

   screen_.Scroll(bufferSize);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == bufferSize - rows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == bufferSize);
}

void ScreenTester::TestAlignTo() // z<CR>, z-, z.
{
   screen_.SetActiveAndVisible(Ui::Screen::MainWindow::Browse);

   int32_t rows = screen_.MaxRows();
   int32_t halfRows = (rows + 1) / 2;
   int32_t bufferSize = screen_.ActiveWindow().BufferSize();

   screen_.ScrollTo(rows);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);

   screen_.AlignTo(Ui::Screen::Location::Top, 0); // zt or z<CR>

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == rows * 2);

   screen_.AlignTo(Ui::Screen::Location::Bottom, 0); // zb or z-

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == rows + 1);

   screen_.AlignTo(Ui::Screen::Location::Centre, 0); // zz or z.

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);

   screen_.AlignTo(Ui::Screen::Location::Specific, 1); // 1G or gg

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == 0);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == rows);

   screen_.AlignTo(Ui::Screen::Location::Top, 2); // zt with count
   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == 1);

   screen_.AlignTo(Ui::Screen::Location::Bottom, (rows * 2) + 1); // zb with count
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() == (rows * 2));

   screen_.AlignTo(Ui::Screen::Location::Centre, rows + 1); // zz with count
   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
}

void ScreenTester::TestSelect() // H, L, M
{
   screen_.SetActiveAndVisible(Ui::Screen::MainWindow::Browse);

   int32_t rows = screen_.MaxRows();
   int32_t halfRows = (rows + 1) / 2;
   int32_t bufferSize = screen_.ActiveWindow().BufferSize();

   screen_.ScrollTo(rows);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);

   screen_.ActiveWindow().Select(Ui::ScrollWindow::Position::First, 1); // H

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().CurrentLine() == screen_.ActiveWindow().FirstLine());

   screen_.ActiveWindow().Select(Ui::ScrollWindow::Position::Middle, 1); // M

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().CurrentLine() == rows - 1);

   screen_.ActiveWindow().Select(Ui::ScrollWindow::Position::Last, 1); // L

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().CurrentLine() == screen_.ActiveWindow().LastLine());

   // H and L with a count

   screen_.ActiveWindow().Select(Ui::ScrollWindow::Position::First, 5);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().CurrentLine() == screen_.ActiveWindow().FirstLine() + 4);

   screen_.ActiveWindow().Select(Ui::ScrollWindow::Position::Last, 5);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().CurrentLine() == screen_.ActiveWindow().LastLine() - 4);

   // H and L with out of bounds count

   screen_.ActiveWindow().Select(Ui::ScrollWindow::Position::First, rows);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().CurrentLine() == screen_.ActiveWindow().LastLine());

   screen_.ActiveWindow().Select(Ui::ScrollWindow::Position::Last, rows);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().CurrentLine() == screen_.ActiveWindow().FirstLine());

   screen_.ActiveWindow().Select(Ui::ScrollWindow::Position::First, rows + 1);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().CurrentLine() == screen_.ActiveWindow().LastLine());

   screen_.ActiveWindow().Select(Ui::ScrollWindow::Position::Last, rows + 1);

   CPPUNIT_ASSERT(screen_.ActiveWindow().FirstLine() == rows - halfRows);
   CPPUNIT_ASSERT(screen_.ActiveWindow().LastLine() + 1 == (rows + halfRows) - 1);
   CPPUNIT_ASSERT(screen_.ActiveWindow().CurrentLine() == screen_.ActiveWindow().FirstLine());
}

CPPUNIT_TEST_SUITE_REGISTRATION(ScreenTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ScreenTester, "screen");
