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

#include <fstream>

char const * const LocalHelpFile    = "doc/help.txt";
char const * const SpecificHelpFile = "/usr/share/vimpc/doc/help.txt";

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
   WINDOW * window = N_WINDOW();

   if ((FirstLine() + line) < buffer_.size())
   {
      mvwaddstr(window, line, 0, buffer_.at(FirstLine() + line).c_str());
   }
}

void HelpWindow::Confirm() const
{
}


void HelpWindow::LoadHelpFile()
{
   std::ifstream testFile(LocalHelpFile);
   std::string   nextLine;
   std::string   file;

   if (testFile.is_open() == true)
   {
      file = LocalHelpFile;
   }
   else
   {
      file = SpecificHelpFile;
   }
   
   testFile.close();

   std::ifstream helpFile(file.c_str());

   if (helpFile.is_open() == true)
   {
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
      Error(4, "Unable to open help file"); 
   }
}

void HelpWindow::Clear()
{
   buffer_.clear();
}

