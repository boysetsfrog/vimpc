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
   CPPUNIT_TEST(isUpper);
   CPPUNIT_TEST(imatch);
   CPPUNIT_TEST(icompare);
   CPPUNIT_TEST(iequals);
   CPPUNIT_TEST(isNumeric);
   CPPUNIT_TEST_SUITE_END();

public:
   AlgorithmTester() { }

public:
   void setUp();
   void tearDown();

protected:
   void isLower();
   void isUpper();
   void imatch();
   void icompare();
   void iequals();
   void isNumeric();

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
   CPPUNIT_ASSERT(Algorithm::isLower("!@#$%(*^&\\{}+-z/") == true);
}

void AlgorithmTester::isUpper()
{
   CPPUNIT_ASSERT(Algorithm::isUpper("this is lowercase") == false);
   CPPUNIT_ASSERT(Algorithm::isUpper("THIS IS NOT")       == true);
   CPPUNIT_ASSERT(Algorithm::isUpper("tHiS is MixEd")     == false);
   CPPUNIT_ASSERT(Algorithm::isUpper("!@#$%(*^&\\{}+-z/") == false);
}

void AlgorithmTester::imatch()
{
   CPPUNIT_ASSERT(Algorithm::imatch("lower", "LOWER", true,  true)  == true);
   CPPUNIT_ASSERT(Algorithm::imatch("lower", "lOwEr", false, true)  == true);
   CPPUNIT_ASSERT(Algorithm::imatch("lower", "lower", true,  true)  == true);
   CPPUNIT_ASSERT(Algorithm::imatch("lower", "LoWer", true,  true)  == true);
   CPPUNIT_ASSERT(Algorithm::imatch("lower", "LOWER", true,  false) == false);
   CPPUNIT_ASSERT(Algorithm::imatch("lower", "lower", true,  false) == true);
}

void AlgorithmTester::icompare()
{
   CPPUNIT_ASSERT(Algorithm::icompare("first", "second")  == true);
   CPPUNIT_ASSERT(Algorithm::icompare("first", "Second")  == true);
   CPPUNIT_ASSERT(Algorithm::icompare("First", "second")  == true);
   CPPUNIT_ASSERT(Algorithm::icompare("first", "second", true,  true)  == true);
   CPPUNIT_ASSERT(Algorithm::icompare("first", "Second", true,  false) == false);
   CPPUNIT_ASSERT(Algorithm::icompare("First", "second", true,  false) == true);
   CPPUNIT_ASSERT(Algorithm::icompare("The first", "second", false,  true) == false);
   CPPUNIT_ASSERT(Algorithm::icompare("the first", "second", true,   true) == true);
   CPPUNIT_ASSERT(Algorithm::icompare("THE FIRST", "second", true,   true) == true);
   CPPUNIT_ASSERT(Algorithm::icompare("The first", "Second", true,  false) == false);
   CPPUNIT_ASSERT(Algorithm::icompare("The First", "second", true,  false) == true);
}

void AlgorithmTester::iequals()
{
   CPPUNIT_ASSERT(Algorithm::iequals("lower", "LOWER")  == true);
   CPPUNIT_ASSERT(Algorithm::iequals("lower", "lOwEr")  == true);
   CPPUNIT_ASSERT(Algorithm::iequals("lower", "lower")  == true);
   CPPUNIT_ASSERT(Algorithm::iequals("lower", "upper")  == false);
}

void AlgorithmTester::isNumeric()
{
   CPPUNIT_ASSERT(Algorithm::isNumeric("99999")   == true);
   CPPUNIT_ASSERT(Algorithm::isNumeric("@#$#@")   == false);
   CPPUNIT_ASSERT(Algorithm::isNumeric("abcddef") == false);
   CPPUNIT_ASSERT(Algorithm::isNumeric("123.45")  == false); //We do not support floats
}

CPPUNIT_TEST_SUITE_REGISTRATION(AlgorithmTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AlgorithmTester, "algorithms");
