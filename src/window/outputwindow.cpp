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

   outputwindow.cpp - selection of the output to use
   */

#include "outputwindow.hpp"

#include "buffers.hpp"
#include "mpdclient.hpp"
#include "regex.hpp"
#include "settings.hpp"
#include "screen.hpp"
#include "mode/search.hpp"
#include "window/console.hpp"

using namespace Ui;

OutputWindow::OutputWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Outputs & outputs, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (settings, screen, "outputs"),
   settings_        (settings),
   client_          (client),
   search_          (search),
   outputs_         (outputs)
{
   outputs_.AddCallback(Main::Buffer_Remove, [this] (Mpc::Outputs::BufferType line) { AdjustScroll(line); });
   outputs_.AddCallback(Main::Buffer_Add,    [this] (Mpc::Outputs::BufferType line) { SoftRedraw(); });
}

OutputWindow::~OutputWindow()
{
   Clear();
}


void OutputWindow::Redraw()
{
   Debug("Output redraw");
   Clear();
   client_.GetAllOutputs();
   SoftRedraw();
   Debug("Output redraw complete");
}

void OutputWindow::SoftRedraw()
{
   Debug("Output softredraw");
   Ui::OutputComparator sorter;

   outputs_.Main::Buffer<Mpc::Output *>::Sort(sorter);

   SetScrollLine(0);
   ScrollTo(0);
   Debug("Output softredraw complete");
}


void OutputWindow::SetOutput(uint32_t line, bool enable, uint32_t count, bool scroll)
{
   for (unsigned int i = 0; i < count; ++i)
   {
      if (line + i < BufferSize())
      {
         client_.SetOutput(outputs_.Get(line + i), enable);
      }
   }
}


int32_t OutputWindow::DetermineColour(uint32_t line) const
{
   int32_t colour = settings_.colours.Song;

   if (line < outputs_.Size())
   {
      if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
          (search_.HighlightSearch() == true))
      {
         Regex::RE expression (".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

         if (expression.CompleteMatch(outputs_.Get(line)->Name()) == true)
         {
            colour = settings_.colours.SongMatch;
         }
      }
   }

   return colour;
}


void OutputWindow::AdjustScroll(Mpc::Output * output)
{
   LimitCurrentSelection();
}


void OutputWindow::Clear()
{
   ScrollTo(0);
   outputs_.Clear();
}
/* vim: set sw=3 ts=3: */
