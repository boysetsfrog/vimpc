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

#include <pcrecpp.h>

#include "buffers.hpp"
#include "callback.hpp"
#include "colour.hpp"
#include "mpdclient.hpp"
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
   typedef Main::CallbackObject<Ui::OutputWindow , Mpc::Outputs::BufferType> WindowCallbackObject;
   typedef Main::CallbackObject<Mpc::Outputs,      Mpc::Outputs::BufferType> OutputCallbackObject;

   outputs_.AddCallback(Main::Buffer_Remove, new WindowCallbackObject  (*this, &Ui::OutputWindow::AdjustScroll));
}

OutputWindow::~OutputWindow()
{
   Clear();
}


void OutputWindow::Redraw()
{
   Clear();
   client_.ForEachOutput(outputs_, static_cast<void (Mpc::Outputs::*)(Mpc::Output *)>(&Mpc::Outputs::Add));
   SoftRedraw();
}

void OutputWindow::SoftRedraw()
{
   Ui::OutputComparator sorter;

   outputs_.Main::Buffer<Mpc::Output *>::Sort(sorter);

   SetScrollLine(0);
   ScrollTo(0);
}

void OutputWindow::Confirm()
{
   if (outputs_.Size() > CurrentLine())
   {
      bool const enabled = !(outputs_.Get(CurrentLine())->Enabled());
      client_.SetOutput(outputs_.Get(CurrentLine()), enabled);
      outputs_.Get(CurrentLine())->SetEnabled(enabled);
   }
}


void OutputWindow::SetOutput(uint32_t line, bool enable, uint32_t count, bool scroll)
{
   for (unsigned int i = 0; i < count; ++i)
   {
      if (line + i < BufferSize())
      {
         client_.SetOutput(outputs_.Get(line + i), enable);
         outputs_.Get(line + i)->SetEnabled(enable);
      }
   }
}


int32_t OutputWindow::DetermineColour(uint32_t line) const
{
   int32_t colour = Colour::Song;

   if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
       (search_.HighlightSearch() == true))
   {
      pcrecpp::RE expression (".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

      if (expression.FullMatch(outputs_.Get(line)->Name()) == true)
      {
         colour = Colour::SongMatch;
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
