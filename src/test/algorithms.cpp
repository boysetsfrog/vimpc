#include <cppunit/extensions/HelperMacros.h>

#include "algorithm.hpp"

class AlgorithmTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(AlgorithmTester);
   CPPUNIT_TEST(isLower);
   CPPUNIT_TEST_SUITE_END();

public:
   AlgorithmTester() { }

public:
   void setUp();
   void tearDown();

protected:
   void isLower();

private:
};

void AlgorithmTester::setUp()
{
}

void AlgorithmTester::tearDown()
{
}

void AlgorithmTester::isLower()
{
   CPPUNIT_ASSERT(Algorithm::isLower("this is lowercase") == true);
   CPPUNIT_ASSERT(Algorithm::isLower("THIS IS NOT")       == false);
   CPPUNIT_ASSERT(Algorithm::isLower("tHiS is MixEd")     == false);
}

CPPUNIT_TEST_SUITE_REGISTRATION(AlgorithmTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AlgorithmTester, "algorithms");
