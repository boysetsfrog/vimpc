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

   scrollwindow.cpp - class representing a scrollable ncurses window
   */

#include "scrollwindow.hpp"

#include <iostream>

#include "colour.hpp"
#include "screen.hpp"

using namespace Ui;

ScrollWindow::ScrollWindow(Ui::Screen & screen, std::string name) :
   Window     (screen.MaxRows(), screen.MaxColumns(), 1, 0),
   settings_  (Main::Settings::Instance()),
   screen_    (screen),
   name_      (name),
   scrollLine_(screen.MaxRows()),
   autoScroll_(false)
{
}

ScrollWindow::~ScrollWindow()
{
}


void ScrollWindow::Print(uint32_t line) const
{
   WINDOW * window = N_WINDOW();

   std::string const BlankLine(Columns(), ' ');
   uint32_t const currentLine(FirstLine() + line);

   int  j         = 0;
   int  align     = -1;
   bool highlight = true;
   bool elided    = false;

   std::string output   = WindowBuffer().PrintString(currentLine);
   std::string stripped = output;

   for (uint32_t i = 0; i < output.size(); )
   {
      if ((output[i] == '$') && ((i + 1) < output.size()))
      {
         if (output[i + 1] == 'E')
         {
            elided = true;
         }
         else if (output[i + 1] == 'R')
         {
            align = j;
         }

         std::string next = "";
         stripped.replace(j, 2, next);
         j += next.size();
         i += 2;
      }
      else
      {
         ++j;
         ++i;
      }
   }

   if (align == -1)
   {
      align = stripped.size();
   }

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattron(window, COLOR_PAIR(DetermineColour(line)));
   }

   mvwprintw(window, line, 0, BlankLine.c_str());
   wmove(window, line, 0);

   for (uint32_t i = 0; i < output.size(); )
   {
      if ((output[i] == '$') && ((i + 1) < output.size()))
      {
         switch (output[i + 1])
         {
            case 'L':
               wprintw(window, "%5d", FirstLine() + line + 1);
               break;

            case 'I':
               if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(FirstLine() + line) == false))
               {
                  wattron(window, COLOR_PAIR(Colour::SongId));
               }
               break;

            case 'D':
               if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(FirstLine() + line) == false))
               {
                  wattron(window, COLOR_PAIR(Colour::Song));
               }
               break;

            case 'R':
               elided = false;
               wmove(window, line, (screen_.MaxColumns() - (stripped.size() - align)));
               break;

            case 'H':
               {
                  if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(FirstLine() + line) == false))
                  {
                     if (highlight == false)
                     {
                        wattron(window, COLOR_PAIR(DetermineColour(line)));
                     }
                     else
                     {
                        wattroff(window, COLOR_PAIR(DetermineColour(line)));
                     }

                     highlight = !highlight;
                  }
               }

             default:
               break;
         }

         i += 2;
      }
      else if ((elided == false) || 
               (getcurx(window) < static_cast<int32_t>(Columns() - 3 - (stripped.size() - align))))
      {
         wprintw(window, "%c", output[i]);
         ++i;
      }
      else 
      {
         wprintw(window, "...");
         ++i;
      }
   }

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      if (highlight == true)
      {
         wattroff(window, COLOR_PAIR(DetermineColour(line)));
      }
   }
}


void ScrollWindow::Resize(int rows, int columns)
{
   if ((scrollLine_ > rows) || (rows > scrollLine_))
   {
      scrollLine_ = FirstLine() + rows;
   }
   if (scrollLine_ < rows)
   {
      scrollLine_ = rows;
   }

   Window::Resize(rows, columns);
}


void ScrollWindow::Scroll(int32_t scrollCount)
{
   uint16_t const newLine = (scrollLine_ + scrollCount);

   if (BufferSize() > Rows())
   {
      if (newLine < Rows())
      {
         scrollLine_ = Rows();
      }
      else if (newLine > BufferSize())
      {
         scrollLine_ = BufferSize();
      }
      else
      {
         scrollLine_ = newLine;
      }
   }
}

void ScrollWindow::ScrollTo(uint16_t scrollLine)
{
   if (scrollLine > BufferSize())
   {
      scrollLine_ = BufferSize();
   }
   else if (BufferSize() >= Rows())
   {
      scrollLine_ = scrollLine + (Rows() / 2);

      if (scrollLine_ < Rows())
      {
         scrollLine_ = Rows();
      }
      else if (scrollLine_ > BufferSize())
      {
         scrollLine_ = BufferSize();
      }
   }
   else
   {
      scrollLine_ = Rows();
   }
}

std::string const & ScrollWindow::Name()
{
   return name_;
}

void ScrollWindow::SetName(std::string const & name)
{
   name_ = name;
}

bool ScrollWindow::Select(Position position, uint32_t count)
{
   if (position == ScrollWindow::First)
   {
      int32_t scroll = FirstLine() -1 + count;

      if (scroll > static_cast<int32_t>(LastLine()))
      {
         scroll = LastLine();
      }

      ScrollTo(scroll);
   }
   else if (position == ScrollWindow::Last)
   {
      int32_t scroll = LastLine() - count;

      if (scroll < static_cast<int32_t>(FirstLine() + 1))
      {
         scroll = FirstLine() + 1;
      }

      ScrollTo(scroll);
   }
   else if (position == ScrollWindow::Middle)
   {
      ScrollTo(FirstLine() + ((LastLine() - FirstLine() - 1) / 2) );
   }

   return true;
}

void ScrollWindow::SetAutoScroll(bool autoScroll)
{
   autoScroll_ = autoScroll;
}

bool ScrollWindow::AutoScroll() const
{
   return autoScroll_;
}


uint32_t ScrollWindow::FirstLine() const
{
   uint16_t result = 0;

   if ((scrollLine_ - Rows()) > 0)
   {
      result = (scrollLine_ - Rows());
   }

   return result;
}

void ScrollWindow::ResetScroll()
{
   scrollLine_ = Rows();
}

uint16_t ScrollWindow::ScrollLine() const
{
   return scrollLine_;
}

void ScrollWindow::SetScrollLine(uint16_t scrollLine)
{
   scrollLine_ = scrollLine;
}


void ScrollWindow::SoftRedrawOnSetting(Setting::ToggleSettings setting)
{
   settings_.RegisterCallback(setting,
      new Main::CallbackObject<Ui::ScrollWindow, bool>(*this, &Ui::ScrollWindow::OnSettingChanged));
}

void ScrollWindow::SoftRedrawOnSetting(Setting::StringSettings setting)
{
   settings_.RegisterCallback(setting,
      new Main::CallbackObject<Ui::ScrollWindow, std::string>(*this, &Ui::ScrollWindow::OnSettingChanged));
}

int32_t ScrollWindow::DetermineColour(uint32_t line) const
{
   return Colour::Song;
}

/* vim: set sw=3 ts=3: */
