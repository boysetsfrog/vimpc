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

   selectwindow.cpp - window that is scrollable and has selectable elements
   */

#include "selectwindow.hpp"

#include "settings.hpp"
#include "screen.hpp"

using namespace Ui;

SelectWindow::SelectWindow(Main::Settings const & settings, Ui::Screen & screen, std::string name) :
   ScrollWindow     (screen, name),
   currentLine_     (0),
   settings_        (settings),
   visualMode_      (false)
{
   currentSelection_.first  = 0;
   currentSelection_.second = 0;

   lastSelection_ = currentSelection_;
   hadSelection_  = false;
}

SelectWindow::~SelectWindow()
{
}


void SelectWindow::Resize(int rows, int columns)
{
   ScrollWindow::Resize(rows, columns);

   if (currentLine_ >= (rows + FirstLine()))
   {
      currentLine_ = FirstLine() + rows - 1;
   }
   else if (currentLine_ <= FirstLine())
   {
      currentLine_ = FirstLine();
      ScrollTo(FirstLine());
   }

   LimitCurrentSelection();
}

void SelectWindow::Scroll(int32_t scrollCount)
{
   currentLine_ += scrollCount;
   LimitCurrentSelection();

   if ((currentLine_ >= scrollLine_) || (currentLine_ < scrollLine_ - screen_.MaxRows()))
   {
      ScrollWindow::Scroll(scrollCount);
   }
}

void SelectWindow::ScrollTo(uint16_t scrollLine)
{
   int64_t oldSelection = currentLine_;
   currentLine_    = (static_cast<int64_t>(scrollLine));
   LimitCurrentSelection();

   if ((currentLine_ == LastLine()) && (currentLine_ - oldSelection == 1))
   {
      ScrollWindow::Scroll(1);
   }
   else if ((currentLine_ == scrollLine_ - screen_.MaxRows()) && (currentLine_ - oldSelection == -1))
   {
      ScrollWindow::Scroll(-1);
   }
   else if ((currentLine_ >= scrollLine_) || (currentLine_ < (scrollLine_ - screen_.MaxRows())))
   {
      ScrollWindow::ScrollTo(scrollLine);
   }
   else if ((scrollLine > BufferSize()) || (scrollLine_ > BufferSize()))
   {
      ScrollWindow::ScrollTo(BufferSize());
   }
}

uint16_t SelectWindow::CurrentLine() const
{
   LimitCurrentSelection();
   return currentLine_;
}

void SelectWindow::Confirm()
{
   UpdateLastSelection();
   visualMode_ = false;
   currentSelection_.first = currentLine_;
}


void SelectWindow::AddLine(uint32_t line, uint32_t count, bool scroll)
{
   UpdateLastSelection();
   visualMode_ = false;
   currentSelection_.first = currentLine_;
}

void SelectWindow::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
   UpdateLastSelection();
   visualMode_ = false;
   currentSelection_.first = currentLine_;
}

void SelectWindow::Escape()
{
   UpdateLastSelection();
   visualMode_ = false;
   currentSelection_.first = currentLine_;
}

void SelectWindow::Visual()
{
   UpdateLastSelection();

   hadSelection_ = true;
   visualMode_   = !visualMode_;
   currentSelection_.first = currentLine_;
}


bool SelectWindow::IsSelected(uint32_t line) const
{
   return (((currentSelection_.first == line) || (currentSelection_.second == line)) ||
           ((currentSelection_.first > line) && (currentSelection_.second < line)) ||
           ((currentSelection_.first < line) && (currentSelection_.second > line)));
}

Ui::Selection SelectWindow::CurrentSelection() const
{
   return currentSelection_;
}

void SelectWindow::ResetSelection()
{
   if (hadSelection_ == true)
   {
      if ((lastSelection_.first > -1) && (lastSelection_.second < BufferSize()))
      {
         visualMode_       = true;
         currentSelection_ = lastSelection_;
         currentLine_      = currentSelection_.second;
         ScrollTo(currentLine_);
      }
   }
}


void SelectWindow::LimitCurrentSelection() const
{
   if (currentLine_ < 0)
   {
      currentLine_ = 0;
   }
   else if ((currentLine_ >= static_cast<int32_t>(BufferSize())) && (BufferSize() > 0))
   {
      currentLine_ = BufferSize() - 1;
   }

   if (visualMode_ == false)
   {
      currentSelection_.first = currentLine_;
   }

   currentSelection_.second = currentLine_;
}

void SelectWindow::PrintSong(int32_t line, int32_t Id, int32_t colour, std::string fmt, Mpc::Song * song) const
{
   //! \TODO this definitely needs some cleaning up
   WINDOW * window = N_WINDOW();
   std::string songString = song->FormatString(fmt);

   int  j         = 0;
   int  align     = -1;
   bool highlight = true;
   bool elided    = false;

   std::string stripped = songString;

   for (uint32_t i = 0; i < songString.size(); )
   {
      if ((songString[i] == '$') && ((i + 1) < songString.size()))
      {
         if (songString[i + 1] == 'E')
         {
            elided = true;
         }
         else if (songString[i + 1] == 'R')
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
      wattron(window, COLOR_PAIR(colour));
   }

   for (uint32_t i = 0; i < songString.size(); )
   {
      if ((songString[i] == '$') && ((i + 1) < songString.size()))
      {
         switch (songString[i + 1])
         {
            case 'R':
               elided = false;
               wmove(window, line, (screen_.MaxColumns() - (stripped.size() - align)));
               break;

            case 'H':
               {
                  if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(Id) == false))
                  {
                     if (highlight == false)
                     {
                        wattron(window, COLOR_PAIR(colour));
                     }
                     else
                     {
                        wattroff(window, COLOR_PAIR(colour));
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
               (getcurx(window) < static_cast<int32_t>(screen_.MaxColumns() - 3 - (stripped.size() - align))))
      {
         wprintw(window, "%c", songString[i]);
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
         wattroff(window, COLOR_PAIR(colour));
      }
   }
}

void SelectWindow::UpdateLastSelection()
{
   if (visualMode_ == true)
   {
      lastSelection_ = currentSelection_;
   }
}


/* vim: set sw=3 ts=3: */
