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
   WINDOW * window = N_WINDOW();

   bool highlight = true;
   bool elided    = false;
   bool bold      = false;
	bool songid    = false;

   uint32_t const currentLine(FirstLine() + line);

   if (currentLine >= WindowBuffer().Size())
	{
      //mvwprintw(window, line, 0, std::string(Columns(), ' ').c_str());
		return;
	}

   std::string output = WindowBuffer().PrintString(currentLine);


   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattron(window, COLOR_PAIR(DetermineColour(line)));
   }


   for (uint32_t i = 0; i < output.size(); i++)
	{
		switch(output[i])
		{
			case '\\':
				i++; // \ should be followed by another letter so lets peek it
				if(i < output.size()) wprintw(window, "%c", output[i]);
				break;
			case '$':
				i++; // $ should be followed by another letter so lets peek it
				if(i < output.size()) switch(output[i])
         	{
         	   case 'L':	// Print the song index
         	      wprintw(window, "%5d", FirstLine() + line + 1);
         	      break;

         	   case 'B':
						bold ? wattroff(window, A_BOLD) : wattron(window, A_BOLD);
         	      bold = !bold;
         	      break;

         	   case 'H':
						if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(FirstLine() + line) == false))
						{
							highlight ? wattroff(window, COLOR_PAIR(DetermineColour(line))) : wattron(window, COLOR_PAIR(DetermineColour(line)));
							highlight = !highlight;
						}
						break;

         	   case 'I':	// $I and $D control the highlight for the song index printing. The only seem to appear there and it seems ...
					case 'D':	// ...asimple toggle is enough. TODO: Discuss is we can completely remove one of the tokens
         	      if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(FirstLine() + line) == false))
         	      {
         	         songid ? wattron(window, COLOR_PAIR(settings_.colours.Song)) : wattron(window, COLOR_PAIR(settings_.colours.SongId));
							songid = !songid;
         	      }
         	      break;

         	   case 'E':
						elided = true;
						break;

					case 'R':
						{
							// Lets see how long the right-aligned part is...
							uint32_t length = 0;
							for(uint32_t j=i; (j < output.size()); j++)
							{
								// Everything except $x and the backslash in \x is printed, so we should count it in the length calculation
								(output[j] != '$' && output[j] != '\\') && length++;

								// If there is a \\ in the code, the length must in increase
								(output[j] == '\\' && ((j+1) < output.size()) && output[j+1] == '\\') && length++;

								// We don't care what there is after the $, so skip it
								(output[j] == '$') && j++;
							}

							// ... and then move so that we can print it nicely.
							uint32_t new_position = Columns() - length+1;
							if(new_position > getcurx(window))
							{
								wprintw(window, "%s", std::string(new_position-getcurx(window), settings_.Get(Setting::SongFillChar).front()).c_str());
							}
							else if(new_position > 4 && elided)
							{
								wmove(window, line, new_position-3);
								wprintw(window, "...");
							}
							else
							{
								wmove(window, line, new_position);
							}
						}
						break;

         	   case 'A':
						if(i+2 < output.size()) // The token "$A50" has two more characters assigned
         	      {
							assert(('0' <= output[i+1]) && (output[i+1] <= '9'));
							assert(('0' <= output[i+2]) && (output[i+2] <= '9'));
							uint32_t percentage = (output[i+1]-'0')*10 + (output[i+2]-'0');

							// Move to the proper tabstop position.
							uint32_t new_position = (Columns()*percentage)/100;
							if(new_position > getcurx(window))
							{
								wprintw(window, "%s", std::string(new_position-getcurx(window), settings_.Get(Setting::SongFillChar).front()).c_str());
							}
							else if(new_position > 4 && elided)
							{
								wmove(window, line, new_position-3);
								wprintw(window, "...");
							}
							else
							{
								wmove(window, line, new_position);
							}

							i += 2; // This skip of i is due to the two extra characters
         	      }
						break;

         	    default:
						wprintw(window, "%c", output[i]);
         	      break;
         	}
				//}}}

				break;

			default:
				{
					// Speed up the rendering by printing as much as possible in one go
					uint32_t j;
					for(j=i; (j < output.size()) && (output[j] != '\\') && (output[j] != '$'); j++); // Find the first part we cannot print in one go...
					waddnstr(window, &output[i], j-i);                                               // ... and print up till there.
					i+=j-i-1;
				}
				break;
		}
	}

	// Reset the highlight and bold if they are set
   settings_.Get(Setting::ColourEnabled) && highlight && wattroff(window, COLOR_PAIR(DetermineColour(line)));
	bold && wattroff(window, A_BOLD);

	// Finish the line
	const int32_t curx = getcurx(window);
	const int32_t remaining_space = Columns()-curx;
	curx && remaining_space && wprintw(window, "%s", std::string(remaining_space, ' ').c_str());

	return;
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
   int64_t const newLine = static_cast<int64_t>(scrollLine_) + scrollCount;

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
