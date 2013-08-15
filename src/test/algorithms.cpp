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
