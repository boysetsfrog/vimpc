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

   windowselector.cpp - window to select other windows
   */

#include "windowselector.hpp"

#include "debug.hpp"
#include "screen.hpp"

using namespace Ui;

WindowSelector::WindowSelector(Main::Settings const & settings, Ui::Screen & screen,
                               Ui::Windows const & windows, Ui::Search const & search) :
   SelectWindow(settings, screen, "windows"),
   windows_    (windows),
   settings_   (settings)
{
}

WindowSelector::~WindowSelector()
{
}


void WindowSelector::Clear()
{
   werase(N_WINDOW());
   ResetScroll();
}


void WindowSelector::Confirm()
{
   if (windows_.Get(CurrentLine()) != static_cast<uint32_t>(screen_.GetWindowFromName(Name())))
   {
      screen_.SetActiveAndVisible(windows_.Get(CurrentLine()));
      screen_.SetVisible(screen_.GetWindowFromName(Name()), false);
   }
}


void WindowSelector::AddLine(uint32_t line, uint32_t count, bool scroll)
{
   int64_t pos1 = CurrentSelection().first;
   int64_t pos2 = CurrentSelection().second;

   if (pos2 < pos1)
   {
      pos2 = pos1;
      pos1 = CurrentSelection().second;
   }

   if (pos1 != pos2)
   {
      count  = pos2 - pos1 + 1;
      line   = pos1;
      scroll = false;
   }

   for (uint32_t i = 0; i < count; ++i)
   {
      if ((line + i) < windows_.Size())
      {
         screen_.SetVisible(static_cast<Ui::Screen::MainWindow>(windows_.Get(line + i)), true);
      }
   }
}

void WindowSelector::AddAllLines()
{
   AddLine(0, windows_.Size());
}

void WindowSelector::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
   int64_t pos1 = CurrentSelection().first;
   int64_t pos2 = CurrentSelection().second;

   if (pos2 < pos1)
   {
      pos2 = pos1;
      pos1 = CurrentSelection().second;
   }

   if (pos1 != pos2)
   {
      count  = pos2 - pos1 + 1;
      line   = pos1;
      scroll = false;
   }

   for (uint32_t i = 0; i < count; ++i)
   {
      if ((line + i) < windows_.Size())
      {
         screen_.SetVisible(static_cast<Ui::Screen::MainWindow>(windows_.Get(line + i)), false);
      }
   }
}

void WindowSelector::DeleteAllLines()
{
   DeleteLine(0, windows_.Size());
}

int32_t WindowSelector::DetermineColour(uint32_t line) const
{
   int32_t colour = settings_.colours.Song;

   if (line + FirstLine() < windows_.Size())
   {
      if (screen_.IsVisible(static_cast<Ui::Screen::MainWindow>(windows_.Get(line + FirstLine()))) == true)
      {
         colour = settings_.colours.FullAdd;
      }
   }

   return colour;
}

/* vim: set sw=3 ts=3: */
