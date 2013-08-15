#include <cppunit/extensions/HelperMacros.h>
#include "settings.hpp"

class SettingsTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(SettingsTester);
   CPPUNIT_TEST(TestDefaults);
   CPPUNIT_TEST(TestToggleSettings);
   CPPUNIT_TEST(TestTurnOffSettings);
   CPPUNIT_TEST(TestTurnOnSettings);
   CPPUNIT_TEST_SUITE_END();

public:
   SettingsTester() :
      settings_(Main::Settings::Instance()) { }

public:
   void setUp();
   void tearDown();

protected:
   void TestDefaults();
   void TestToggleSettings();
   void TestTurnOffSettings();
   void TestTurnOnSettings();

protected:
   bool IsToggleDefaultValues();
   bool IsNotToggleDefaultValues();
   bool IsToggleOn();
   bool IsToggleOff();

private:
   Main::Settings & settings_; 

};

void SettingsTester::setUp()
{
}

void SettingsTester::tearDown()
{
}

void SettingsTester::TestDefaults()
{
   CPPUNIT_ASSERT(IsToggleDefaultValues() == true);
}

void SettingsTester::TestToggleSettings()
{
#define X(a, b, c) settings_.SetSingleSetting(std::string(b) + "!");
   TOGGLE_SETTINGS
#undef X

   CPPUNIT_ASSERT(IsToggleDefaultValues() == false);
   CPPUNIT_ASSERT(IsNotToggleDefaultValues() == true);
}

void SettingsTester::TestTurnOffSettings()
{
#define X(a, b, c) settings_.SetSingleSetting("no" + std::string(b));
   TOGGLE_SETTINGS
#undef X

   CPPUNIT_ASSERT(IsToggleOff() == true);
}

void SettingsTester::TestTurnOnSettings()
{
#define X(a, b, c) settings_.SetSingleSetting(std::string(b));
   TOGGLE_SETTINGS
#undef X

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

   //CPPUNIT_ASSERT(true == false);
   //CPPUNIT_FAIL("FAIL");
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
