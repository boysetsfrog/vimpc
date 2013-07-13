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
   Debug("Window Selected:" + windows_.Get(CurrentLine()));

   if (windows_.Get(CurrentLine()) != Name())
   {
      screen_.SetActiveAndVisible(screen_.GetWindowFromName(windows_.Get(CurrentLine())));
      screen_.SetVisible(screen_.GetWindowFromName(Name()), false);
   }
}


void WindowSelector::AddLine(uint32_t line, uint32_t count, bool scroll)
{
   for (int i = 0; i < count; ++i)
   {
      screen_.SetVisible(screen_.GetWindowFromName(windows_.Get(line + i)), true);
   }
}

void WindowSelector::AddAllLines()
{
   AddLine(0, windows_.Size());
}

void WindowSelector::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
   for (int i = 0; i < count; ++i)
   {
      screen_.SetVisible(screen_.GetWindowFromName(windows_.Get(line + i)), false);
   }
}

void WindowSelector::DeleteAllLines()
{
   DeleteLine(0, windows_.Size());
}

int32_t WindowSelector::DetermineColour(uint32_t line) const
{
   int32_t colour = settings_.colours.Song;

   if (screen_.IsVisible(screen_.GetWindowFromName(windows_.Get(line + FirstLine()))) == true)
   {
      colour = settings_.colours.FullAdd;
   }

   return colour;
}

/* vim: set sw=3 ts=3: */
