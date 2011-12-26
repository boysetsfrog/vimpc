/*
   Vimpc
   Copyright (C) 2010 - 2011 Nathan Sweetman

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

   help.cpp - window to display help about commands
   */

#include "help.hpp"

#ifndef HELP_DIRECTORY
#define HELP_DIRECTORY ""
#endif

#include "algorithm.hpp"
#include "error.hpp"
#include "screen.hpp"
#include "settings.hpp"

#include <algorithm>
#include <fstream>
#include <string>

char const * const Local    = "doc";
char const * const Specific = HELP_DIRECTORY;
char const * const HelpFile = "/help.txt";

using namespace Ui;

HelpWindow::HelpWindow(Main::Settings const & settings, Ui::Screen & screen) :
   ScrollWindow     (screen, "help"),
   settings_        (settings),
   help_            ()
{
   LoadHelpFile();
}

HelpWindow::~HelpWindow()
{
}

void HelpWindow::Redraw()
{
   Clear();
   LoadHelpFile();
}

void HelpWindow::Print(uint32_t line) const
{
   WINDOW * window = N_WINDOW();

   if ((FirstLine() + line) < help_.Size())
   {
      std::string currentLine = help_.Get(FirstLine() + line);

      if ((FirstLine() == 0) && (line == 0))
      {
         if (settings_.ColourEnabled() == true)
         {
            wattron(window, COLOR_PAIR(BLUEONDEFAULT));
         }

         wattron(window, A_BOLD);
      }
      else if (Algorithm::isUpper(currentLine) == true)
      {
         if (settings_.ColourEnabled() == true)
         {
            wattron(window, COLOR_PAIR(REDONDEFAULT));
         }
      }

      if (currentLine.find('|') != std::string::npos)
      {
         std::string firstHalf = currentLine.substr(0, currentLine.find_last_of('|') - 1);
         std::string lastHalf = currentLine.substr(currentLine.find_last_of('|') + 1);

         mvwaddstr(window, line, 0, firstHalf.c_str());
         waddstr(window, lastHalf.c_str());
      }
      else
      {
         mvwaddstr(window, line, 0, currentLine.c_str());
      }
   }

   if (settings_.ColourEnabled() == true)
   {
      wattroff(window, COLOR_PAIR(REDONDEFAULT) | COLOR_PAIR(BLUEONDEFAULT));
   }

   if ((FirstLine() == 0) && (line == 0))
   {
      wattroff(window, A_BOLD);
   }
}

void HelpWindow::Confirm()
{
}


void HelpWindow::LoadHelpFile()
{
   std::string testFile(Local);

   testFile += HelpFile;

   std::ifstream testStream(testFile.c_str());
   std::string   file;

   if (testStream.is_open() == true)
   {
      file = Local;
   }
   else
   {
      file = Specific;
   }

   file += HelpFile;

   testStream.close();

   std::ifstream helpFile(file.c_str());

   if (helpFile.is_open() == true)
   {
      std::string nextLine;

      while (helpFile.eof() == false)
      {
         getline(helpFile, nextLine);

         if (helpFile.eof() == false)
         {
            help_.Add(nextLine);
         }
      }

      helpFile.close();
   }
   else
   {
      Error(ErrorNumber::HelpFileNonexistant, "Unable to open help file");
   }
}

void HelpWindow::Clear()
{
   help_.Clear();
}

/* vim: set sw=3 ts=3: */
