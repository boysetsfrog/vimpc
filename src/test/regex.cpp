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

#include "regex.hpp"

class RegexTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(RegexTester);
   CPPUNIT_TEST(trim);
   CPPUNIT_TEST(replaceAll);
   CPPUNIT_TEST_SUITE_END();

public:
   RegexTester() { }

public:
   void setUp();
   void tearDown();

protected:
   void trim();
   void replaceAll();

private:
};

void RegexTester::setUp()
{
}

void RegexTester::tearDown()
{
}

void RegexTester::trim()
{
	std::string data = "   word";
	Regex::RE::Trim(data);
   CPPUNIT_ASSERT((data == "word"));

	data = "word   ";
	Regex::RE::Trim(data);
   CPPUNIT_ASSERT((data == "word"));

	data = "   word   ";
	Regex::RE::Trim(data);
   CPPUNIT_ASSERT((data == "word"));

}

void RegexTester::replaceAll()
{
	Regex::RE regex("a");
	std::string data = "ababa";
	regex.ReplaceAll("j", data);

   CPPUNIT_ASSERT((data == "jbjbj"));
}

CPPUNIT_TEST_SUITE_REGISTRATION(RegexTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(RegexTester, "regex");
