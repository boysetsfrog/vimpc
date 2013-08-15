#include <cppunit/extensions/HelperMacros.h>

#include "settings.hpp"
#include "window/debug.hpp"

class SettingsTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(SettingsTester);
   CPPUNIT_TEST(TestTurnOnSettings);
   CPPUNIT_TEST(TestTurnOffSettings);
   CPPUNIT_TEST(TestToggleSettings);
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

protected:
   void TurnOnSettings();
   void TurnOffSettings();

   bool IsToggleDefaultValues();
   bool IsNotToggleDefaultValues();
   bool IsToggleOn();
   bool IsToggleOff();

private:
   Main::Settings & settings_; 
   std::map<std::string, bool> boolvalues_;
};

void SettingsTester::setUp()
{
   settings_.DisableCallbacks();

   for (int i = 0; i < (int) Setting::ToggleCount; ++i)
   {
      boolvalues_[settings_.Name((Setting::ToggleSettings) i)] 
         = settings_.Get((Setting::ToggleSettings) i);
   }
}

void SettingsTester::tearDown()
{
   for (int i = 0; i < (int) Setting::ToggleCount; ++i)
   {
      settings_.Set((Setting::ToggleSettings) i, 
         boolvalues_[settings_.Name((Setting::ToggleSettings) i)]);
   }

   settings_.EnableCallbacks();
}

void SettingsTester::TestToggleSettings()
{
   TurnOffSettings();

#define X(a, b, c) settings_.SetSingleSetting(std::string(b) + "!");
   TOGGLE_SETTINGS
#undef X

   CPPUNIT_ASSERT(IsToggleOn() == true);
   CPPUNIT_ASSERT(IsToggleOff() == false);

#define X(a, b, c) settings_.SetSingleSetting(std::string(b) + "!");
   TOGGLE_SETTINGS
#undef X

   CPPUNIT_ASSERT(IsToggleOn() == false);
   CPPUNIT_ASSERT(IsToggleOff() == true);
}

void SettingsTester::TurnOffSettings()
{
#define X(a, b, c) settings_.SetSingleSetting("no" + std::string(b));
   TOGGLE_SETTINGS
#undef X
}

void SettingsTester::TurnOnSettings()
{
#define X(a, b, c) settings_.SetSingleSetting(std::string(b));
   TOGGLE_SETTINGS
#undef X
}

void SettingsTester::TestTurnOffSettings()
{
   TurnOffSettings();
   CPPUNIT_ASSERT(IsToggleOff() == true);
}

void SettingsTester::TestTurnOnSettings()
{
   TurnOnSettings();
   CPPUNIT_ASSERT(IsToggleOn() == true);
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

CPPUNIT_TEST_SUITE_REGISTRATION(SettingsTester);
