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

   settings.cpp - tests for settings code
   */

#include <cppunit/extensions/HelperMacros.h>

#include "regex.hpp"
#include "settings.hpp"
#include "window/debug.hpp"
#include "window/error.hpp"
#include "window/result.hpp"

class SettingsTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(SettingsTester);
   CPPUNIT_TEST(TestTurnOnSettings);
   CPPUNIT_TEST(TestTurnOffSettings);
   CPPUNIT_TEST(TestToggleSettings);
   CPPUNIT_TEST(TestStringSetting);
   CPPUNIT_TEST_SUITE_END();

public:
   SettingsTester() :
      settings_(Main::Settings::Instance()) { }

public:
   void setUp();
   void tearDown();

protected:
   void TestToggleSettings();
   void TestTurnOffSettings();
   void TestTurnOnSettings();
   void TestStringSetting();

protected:
   void TurnOnSettings();
   void TurnOffSettings();

   bool IsToggleDefaultValues();
   bool IsNotToggleDefaultValues();
   bool IsToggleOn();
   bool IsToggleOff();
   bool AreToggleValuesDisplayedCorrectly();

private:
   Main::Settings & settings_;
   std::map<std::string, bool> boolvalues_;
};

void SettingsTester::setUp()
{
   Ui::ErrorWindow::Instance().ClearError();
   Ui::ResultWindow::Instance().ClearResult();
   settings_.DisableCallbacks();

   for (int i = 0; i < static_cast<int>(Setting::ToggleCount); ++i)
   {
      boolvalues_[settings_.Name(static_cast<Setting::ToggleSettings>(i))]
         = settings_.Get(static_cast<Setting::ToggleSettings>(i));
   }
}

void SettingsTester::tearDown()
{
   for (int i = 0; i < static_cast<int>(Setting::ToggleCount); ++i)
   {
      settings_.Set(static_cast<Setting::ToggleSettings>(i),
         boolvalues_[settings_.Name(static_cast<Setting::ToggleSettings>(i))]);
   }

   settings_.EnableCallbacks();
   Ui::ErrorWindow::Instance().ClearError();
   Ui::ResultWindow::Instance().ClearResult();
}

void SettingsTester::TestToggleSettings()
{
   // Turn off all the settings
   TurnOffSettings();

   // Toggle every single setting
#define X(a, b, c) settings_.Set(std::string(b) + "!");
   TOGGLE_SETTINGS
#undef X

   // Ensure that all settings are now on
   CPPUNIT_ASSERT(IsToggleOn() == true);
   CPPUNIT_ASSERT(IsToggleOff() == false);

   // Toggle every single setting
#define X(a, b, c) settings_.Set(std::string(b) + "!");
   TOGGLE_SETTINGS
#undef X

   // Ensure that all settings are now off
   CPPUNIT_ASSERT(IsToggleOn() == false);
   CPPUNIT_ASSERT(IsToggleOff() == true);

   // Try and toggle an invalid setting, ensure we error out
   settings_.Set("noinvalidsetting!");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();
}

void SettingsTester::TurnOffSettings()
{
#define X(a, b, c) settings_.Set("no" + std::string(b));
   TOGGLE_SETTINGS
#undef X
}

void SettingsTester::TurnOnSettings()
{
#define X(a, b, c) settings_.Set(std::string(b));
   TOGGLE_SETTINGS
#undef X
}

void SettingsTester::TestTurnOffSettings()
{
   // Check that all settings can be turned off
   TurnOffSettings();
   CPPUNIT_ASSERT(IsToggleOff() == true);

   // Ensure that :set <setting>? works properly
   CPPUNIT_ASSERT(AreToggleValuesDisplayedCorrectly() == true);

   // Try and set an invalid setting off
   settings_.Set("noinvalidsetting");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();
}

void SettingsTester::TestTurnOnSettings()
{
   // Check that all settings can be turned on
   TurnOnSettings();
   CPPUNIT_ASSERT(IsToggleOn() == true);

   // Ensure that :set <setting>? works properly
   CPPUNIT_ASSERT(AreToggleValuesDisplayedCorrectly() == true);

   // Try and set an invalid setting on
   settings_.Set("invalidsetting");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();
}

void SettingsTester::TestStringSetting()
{
   // Change the window setting use the name functions
   std::string window = settings_.Get(Setting::Window);
   settings_.Set(settings_.Name(Setting::Window) + " test");
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == "test");
   settings_.Set(settings_.Name(Setting::Window) + " " + window);
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == window);

   // Set the add setting
   std::string position = settings_.Get(Setting::AddPosition);
   settings_.Set("add next");
   CPPUNIT_ASSERT(settings_.Get(Setting::AddPosition) == "next");

   // Ensure the add setting is printed correctly
   Ui::ResultWindow::Instance().ClearResult();
   settings_.Set("add?");
   CPPUNIT_ASSERT(Ui::ResultWindow::Instance().HasResult() == true);

   std::string setting, value;
   std::string result = Ui::ResultWindow::Instance().GetResult();

   Regex::RE const strip("^\\s*([^\\s=]*)=(.*)$");
   strip.Capture(result.c_str(), &setting, &value);

   Debug("before the fail %s %s", setting.c_str(), value.c_str());

   CPPUNIT_ASSERT((setting == "add") && (value == "next"));

   // Try and set the add setting to something invalid
   settings_.Set("add invalid");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();
   CPPUNIT_ASSERT(settings_.Get(Setting::AddPosition) == "next");

   // Ensure that the add setting is still printed the same as before
   // i.e. that it has not changed
   result = Ui::ResultWindow::Instance().GetResult();
   strip.Capture(result.c_str(), &setting, &value);
   CPPUNIT_ASSERT((setting == "add") && (value == "next"));

   // Set the add setting back to normal
   settings_.Set("add " + position);
   CPPUNIT_ASSERT(settings_.Get(Setting::AddPosition) == position);

   // Try and set a setting that does not exist
   settings_.Set("notarealsetting notarealvalue");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();
}

bool SettingsTester::IsToggleDefaultValues()
{
   return
   (
#define X(a, b, c) settings_.Get(Setting::a) == c &&
      TOGGLE_SETTINGS
#undef X
      true
   );
}

bool SettingsTester::IsNotToggleDefaultValues()
{
   return
   (
#define X(a, b, c) settings_.Get(Setting::a) == !c &&
      TOGGLE_SETTINGS
#undef X
      true
   );
}


bool SettingsTester::IsToggleOn()
{
   return
   (
#define X(a, b, c) settings_.Get(Setting::a) == true &&
      TOGGLE_SETTINGS
#undef X
      true
   );
}

bool SettingsTester::IsToggleOff()
{
   return
   (
#define X(a, b, c) settings_.Get(Setting::a) == false &&
      TOGGLE_SETTINGS
#undef X
      true
   );
}


bool SettingsTester::AreToggleValuesDisplayedCorrectly()
{
   for (int i = 0; i < static_cast<int>(Setting::ToggleCount); ++i)
   {
      // Run :set <setting>? to have the setting printed
      Ui::ResultWindow::Instance().ClearResult();
      settings_.Set(settings_.Name(static_cast<Setting::ToggleSettings>(i)) + "?");

      // Ensure that there are no errors but that there is a printed result
      CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
      CPPUNIT_ASSERT(Ui::ResultWindow::Instance().HasResult() == true);

      std::string Result = Ui::ResultWindow::Instance().GetResult();

      Regex::RE const strip("^\\s*([^\\s]*)$");
      strip.Capture(Result.c_str(), &Result);

      // Ensure settings turned on are printed as <setting> those off are
      // printed as <nosetting>
      if (settings_.Get(static_cast<Setting::ToggleSettings>(i)) == true)
      {
         CPPUNIT_ASSERT(settings_.Name(static_cast<Setting::ToggleSettings>(i)) == Result);
      }
      else
      {
         CPPUNIT_ASSERT(("no" + settings_.Name(static_cast<Setting::ToggleSettings>(i))) == Result);
      }
   }

   return true;
}

CPPUNIT_TEST_SUITE_REGISTRATION(SettingsTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(SettingsTester, "settings");
