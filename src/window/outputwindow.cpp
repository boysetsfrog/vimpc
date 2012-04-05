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

OutputWindow::OutputWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Ui::Search const & search) :
   SelectWindow     (settings, screen, "outputs"),
   settings_        (settings),
   client_          (client),
   search_          (search),
   outputs_         (Main::Outputs())
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

   Ui::OutputComparator sorter;

   outputs_.Main::Buffer<Mpc::Output *>::Sort(sorter);

   SetScrollLine(0);
   ScrollTo(0);
}

void OutputWindow::Print(uint32_t line) const
{
   uint32_t printLine = line + FirstLine();

   if (printLine < BufferSize())
   {
      WINDOW * window = N_WINDOW();
      int32_t  colour = DetermineColour(printLine);

      if (settings_.ColourEnabled() == true)
      {
         wattron(window, COLOR_PAIR(colour));
      }

      if (printLine == CurrentLine())
      {
         wattron(window, A_REVERSE);
      }

      wattron(window, A_BOLD);

      mvwhline(window,  line, 0, ' ', screen_.MaxColumns());
      mvwaddstr(window, line, 1, outputs_.Get(printLine)->Name().c_str());

      if (outputs_.Get(printLine)->Enabled() == true)
      {
         waddstr(window, " [Enabled]");
      }

      wattroff(window, A_BOLD);
      wattroff(window, A_REVERSE);

      if (settings_.ColourEnabled() == true)
      {
         wattroff(window, COLOR_PAIR(colour));
      }
   }
}

void OutputWindow::Left(Ui::Player & player, uint32_t count)
{
   //player.SkipSong(Ui::Player::Previous, count);
}

void OutputWindow::Right(Ui::Player & player, uint32_t count)
{
   //player.SkipSong(Ui::Player::Next, count);
}

void OutputWindow::Confirm()
{
   if (outputs_.Size() > CurrentLine())
   {
      client_.EnableOutput(outputs_.Get(CurrentLine()));
      outputs_.Get(CurrentLine())->SetEnabled(true);
   }
}

void OutputWindow::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
   for (unsigned int i = 0; i < count; ++i)
   {
      if (line + i < BufferSize())
      {
         client_.DisableOutput(outputs_.Get(line + i));
         outputs_.Get(line + i)->SetEnabled(false);
      }
   }
}

void OutputWindow::DeleteAllLines()
{
   DeleteLine(0, BufferSize(), false);
}

int32_t OutputWindow::DetermineColour(uint32_t line) const
{
   int32_t colour = Colour::Song;

   if ((search_.LastSearchString() != "") && (settings_.HightlightSearch() == true))
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
