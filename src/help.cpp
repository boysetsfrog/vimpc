/*
   Vimpc
   Copyright (C) 2010 Nathan Sweetman

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

#include "error.hpp"
#include "screen.hpp"

#include <algorithm>
#include <fstream>
#include <string>

char const * const Local    = "doc";
char const * const Specific = HELP_DIRECTORY;
char const * const HelpFile = "/help.txt";

using namespace Ui;

HelpWindow::HelpWindow(Ui::Screen const & screen) :
   ScrollWindow     (screen)
{
   // \todo instead of using a help.txt file, may want to add
   // descriptions for each normal and command mode function
   // and then just iterate over their handler tables printing them out

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
   // \todo add specific help file formatting?

   WINDOW * window = N_WINDOW();

   if ((FirstLine() + line) < buffer_.size())
   {
      std::string currentLine = buffer_.at(FirstLine() + line);

      std::string currentLineUpper = currentLine;
      std::transform(currentLineUpper.begin(), currentLineUpper.end(), currentLineUpper.begin(), ::toupper);

      if ((FirstLine() == 0) && (line == 0))
      {
         wattron(window, A_BOLD | COLOR_PAIR(REDONDEFAULT));
      }
      else if (currentLine.compare(currentLineUpper) == 0)
      {
         wattron(window, A_BOLD | COLOR_PAIR(BLUEONDEFAULT));
      }

      if (currentLine.find('|') != std::string::npos)
      {
         std::string firstHalf = currentLine.substr(0, currentLine.find_last_of('|') - 1);
         std::string lastHalf = currentLine.substr(currentLine.find_last_of('|') + 1);

         wattron(window, A_BOLD | COLOR_PAIR(GREENONDEFAULT));
         mvwaddstr(window, line, 0, firstHalf.c_str());

         wattroff(window, A_BOLD | COLOR_PAIR(GREENONDEFAULT));
         waddstr(window, lastHalf.c_str());
      }
      else
      {
         mvwaddstr(window, line, 0, currentLine.c_str());
      }
   }

   wattroff(window, A_BOLD | COLOR_PAIR(REDONDEFAULT) | COLOR_PAIR(BLUEONDEFAULT));
}

void HelpWindow::Confirm() const
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
            buffer_.push_back(nextLine);
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
   buffer_.clear();
}

