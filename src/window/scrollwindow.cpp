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

#include "screen.hpp"
#include "window/debug.hpp"
#include <cassert>

using namespace Ui;

ScrollWindow::ScrollWindow(Ui::Screen & screen, std::string name) :
   settings_  (Main::Settings::Instance()),
   screen_    (screen),
   name_      (name),
   window_    (screen.W_MainWindow()),
   rows_      (screen.MaxRows()),
   cols_      (screen.MaxColumns()),
   scrollLine_(screen.MaxRows()),
   autoScroll_(false),
   enabled_   (true)
{
}

ScrollWindow::~ScrollWindow()
{
}


void ScrollWindow::Print(uint32_t line) const
{
   //! \TODO needs cleaning up
   WINDOW * window = N_WINDOW();

   std::string const BlankLine(Columns(), ' ');
   uint32_t const currentLine(FirstLine() + line);

   int  j         = 0;
   int  align     = -1;
   bool highlight = true;
   bool elided    = false;
   bool bold      = false;
   bool escape    = false;

   std::string output   = "";

   if (currentLine < WindowBuffer().Size())
   {
      output = WindowBuffer().PrintString(currentLine);
   }
   else
   {
      mvwprintw(window, line, 0, BlankLine.c_str());
   }

   std::string stripped = output;

   for (uint32_t i = 0; i < output.size(); )
   {
      if (output[i] == '\\')
      {
         stripped.replace(j, 1, "");
         ++i;
      }
      else if ((output[i] == '$') && ((i + 1) < output.size()))
      {
			uint32_t eat = 2;
         if (output[i + 1] == 'E')
         {
            elided = true;
         }
         else if (output[i + 1] == 'R')
         {
            align = j;
         }
         else if (output[i + 1] == 'A') // The align argument has two digits of argument which --as the $X tokens themselv-- shall not be printed
         {
				eat +=2;
         }

         stripped.erase(j, eat);
         i += eat;
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
      if (output[i] == '\\')
      {
         escape = true;
         ++i;
      }
      else if ((output[i] == '$') && ((i + 1) < output.size()) && (escape == false))
      {
         switch (output[i + 1])
         {
            case 'L':
               wprintw(window, "%5d", FirstLine() + line + 1);
               break;

            case 'B':
               if (bold == false)
               {
                  wattron(window, A_BOLD);
               }
               else
               {
                  wattroff(window, A_BOLD);
               }
               bold = !bold;
               break;

            case 'I':
               if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(FirstLine() + line) == false))
               {
                  wattron(window, COLOR_PAIR(settings_.colours.SongId));
               }
               break;

            case 'D':
               if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(FirstLine() + line) == false))
               {
                  wattron(window, COLOR_PAIR(settings_.colours.Song));
               }
               break;

            case 'R':
               {
                  elided = false;
                  int y, x;
                  getyx(window, y, x);

                  int width = (Columns() - (stripped.size() - align)) - x;

                  if (width > 0)
                  {
                     wprintw(window, "%s", std::string(width, settings_.Get(Setting::SongFillChar).front()).c_str());
                  }

                  wmove(window, line, Columns() - (stripped.size() - align));
                  break;
               }

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
						break;
               }
            case 'A':
					if(i+3 < output.size()) // The token "$A50" has two more characters assigned
               {
						assert(('0' <= output[i+2]) && (output[i+2] <= '9'));
						assert(('0' <= output[i+3]) && (output[i+3] <= '9'));
						uint32_t percentage = (output[i + 2]-'0')*10 + (output[i + 3]-'0');

						uint32_t new_position = (Columns()*percentage)/100;
						uint32_t old_position = getcurx(window);
						i += 2; // This increase of i is due to the two extra characters

						if(new_position > old_position)
						{
							wprintw(window, "%s", std::string(new_position-getcurx(window), settings_.Get(Setting::SongFillChar).front()).c_str());
						}
						else if(new_position > 4 && elided)
						{
							wmove(window, getcury(window), new_position-3);
							wprintw(window, "...");
						}
						else
						{
							wmove(window, getcury(window), new_position);
						}

						break;
               }

             default:
               break;
         }

         i += 2;

      }
      else if ((elided == false) ||
               (getcurx(window) < static_cast<int32_t>(Columns() - 3 - (stripped.size() - align))))
      {
         escape = false;
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

   if (bold == true)
   {
      wattroff(window, A_BOLD);
   }
}


void ScrollWindow::Resize(uint32_t rows, uint32_t columns)
{
   rows_ = rows;
   cols_ = columns;

   if ((scrollLine_ > rows) || (rows > scrollLine_))
   {
      scrollLine_ = FirstLine() + rows;
   }
   if (scrollLine_ < rows)
   {
      scrollLine_ = rows;
   }
}


void ScrollWindow::Scroll(int32_t scrollCount)
{
   int64_t const newLine = (scrollLine_ + scrollCount);

   if (BufferSize() > static_cast<uint32_t>(Rows()))
   {
      if (newLine < static_cast<uint32_t>(Rows()))
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

void ScrollWindow::ScrollTo(uint32_t scrollLine)
{
   if (scrollLine > BufferSize())
   {
      scrollLine_ = BufferSize();
   }
   else if (BufferSize() >= static_cast<uint32_t>(Rows()))
   {
      scrollLine_ = scrollLine + (Rows() / 2);

      if (scrollLine_ < static_cast<uint32_t>(Rows()))
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

std::string const & ScrollWindow::Name() const
{
   return name_;
}

void ScrollWindow::SetName(std::string const & name)
{
   name_ = name;
}

bool ScrollWindow::Select(Position position, uint32_t count)
{
   int64_t scroll = CurrentLine();

   if (position == ScrollWindow::First)
   {
      scroll = (FirstLine() - 1) + count;

      if (scroll > static_cast<int64_t>(LastLine()))
      {
         scroll = LastLine();
      }
   }
   else if (position == ScrollWindow::Last)
   {
      scroll = (LastLine() + 1) - count;

      if (scroll < static_cast<int64_t>(FirstLine()))
      {
         scroll = FirstLine();
      }

   }
   else if (position == ScrollWindow::Middle)
   {
      scroll = FirstLine() + ((LastLine() - FirstLine()) / 2);
   }

   ScrollTo(scroll);

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
   uint32_t result = 0;

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

uint32_t ScrollWindow::ScrollLine() const
{
   return scrollLine_;
}

void ScrollWindow::SetScrollLine(uint32_t scrollLine)
{
   scrollLine_ = scrollLine;
}


void ScrollWindow::SoftRedrawOnSetting(Setting::ToggleSettings setting)
{
   settings_.RegisterCallback(setting, [this] (bool Value) { OnSettingChanged(Value); });
}

void ScrollWindow::SoftRedrawOnSetting(Setting::StringSettings setting)
{
   settings_.RegisterCallback(setting, [this] (std::string Value) { OnSettingChanged(Value); });
}

int32_t ScrollWindow::DetermineColour(uint32_t line) const
{
   return settings_.colours.Song;
}

/* vim: set sw=3 ts=3: */
