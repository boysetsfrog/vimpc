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
#include "window/songwindow.hpp"

class WindowTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(WindowTester);
   CPPUNIT_TEST(VisualTest);

   CPPUNIT_TEST_SUITE_END();

public:
   WindowTester() :
      screen_(*Main::Tester::Instance().Screen) { }

public:
   void setUp();
   void tearDown();

protected:
   void VisualTest();

private:
   Ui::Screen & screen_;
   Ui::SongWindow * window_;
   int32_t windowId_;
};

void WindowTester::setUp()
{
   window_ = screen_.CreateSongWindow("test");
   Main::Library().ForEachSong([this] (Mpc::Song * song) { window_->Add(song); });
   windowId_ = screen_.GetWindowFromName(window_->Name());
   screen_.SetActiveAndVisible(windowId_);
}

void WindowTester::tearDown()
{
   window_->Clear();
   screen_.SetVisible(windowId_, false);
}

void WindowTester::VisualTest()
{
   window_->Visual(); // v
   screen_.ScrollTo(9); // 10G

   CPPUNIT_ASSERT(window_->InVisualMode());
   CPPUNIT_ASSERT(window_->CurrentSelection().first == 0);
   CPPUNIT_ASSERT(window_->CurrentSelection().second == 9);

   window_->Visual(); //v
   CPPUNIT_ASSERT(window_->InVisualMode() == false);

   window_->ResetSelection(); // gv
   CPPUNIT_ASSERT(window_->InVisualMode());
   CPPUNIT_ASSERT(window_->CurrentSelection().first == 0);
   CPPUNIT_ASSERT(window_->CurrentSelection().second == 9);

   window_->SwitchVisualEnd();
   CPPUNIT_ASSERT(window_->InVisualMode());
   CPPUNIT_ASSERT(window_->CurrentSelection().first == 9);
   CPPUNIT_ASSERT(window_->CurrentSelection().second == 0);

   window_->SwitchVisualEnd();
   CPPUNIT_ASSERT(window_->InVisualMode());
   CPPUNIT_ASSERT(window_->CurrentSelection().first == 0);
   CPPUNIT_ASSERT(window_->CurrentSelection().second == 9);
}

CPPUNIT_TEST_SUITE_REGISTRATION(WindowTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(WindowTester, "window");
