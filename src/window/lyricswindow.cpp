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

   lyricswindow.cpp - class representing a scrollable ncurses window
   */

#include "lyricswindow.hpp"

#include "events.hpp"
#include "lyricsloader.hpp"
#include "mpdclient.hpp"
#include "screen.hpp"
#include "vimpc.hpp"
#include "buffer/playlist.hpp"
#include "mode/search.hpp"

#include <sstream>

using namespace Ui;
using namespace Main;

LyricsWindow::LyricsWindow(std::string const & URI, Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search, std::string name) :
   SelectWindow     (settings, screen, name),
   m_URI            (URI),
   settings_        (settings),
   search_          (search),
   lyrics_          ()
{
   Vimpc::EventHandler(Event::LyricsLoaded, [this] (EventData const & Data) { Redraw(); });

   Vimpc::EventHandler(Event::LyricsPercent, [this] (EventData const & Data) 
   { 
       uint32_t end = lyrics_.Size();
       this->ScrollTo((end*(Data.value+10))/100); 
   });

   LoadLyrics();
   Redraw();
}

LyricsWindow::~LyricsWindow()
{
}

void LyricsWindow::Print(uint32_t line) const
{
   WINDOW * window = N_WINDOW();

   std::string const BlankLine(Columns(), ' ');
   mvwprintw(window, line, 0, BlankLine.c_str());
   wmove(window, line, 0);

   if ((FirstLine() == 0) && (line == 0))
   {
      wattron(window, A_BOLD);
   }

   if ((FirstLine() + line) < lyrics_.Size())
   {
      std::string currentLine = lyrics_.Get(FirstLine() + line);

      if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
          (search_.HighlightSearch() == true))
      {
         Regex::RE const expression(".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

         if (expression.CompleteMatch(currentLine))
         {
            wattron(window, COLOR_PAIR(settings_.colours.SongMatch));
         }
      }

      mvwaddstr(window, line, 0, currentLine.c_str());

      if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
          (search_.HighlightSearch() == true))
      {
         Regex::RE const expression(".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

         if (expression.CompleteMatch(currentLine))
         {
            wattroff(window, COLOR_PAIR(settings_.colours.SongMatch));
         }
      }
   }

   if ((FirstLine() == 0) && (line == 0))
   {
      wattroff(window, A_BOLD);
   }
}

void LyricsWindow::LoadLyrics()
{
   Mpc::Song * song = Main::Library().Song(m_URI);
   Main::LyricsLoader::Instance().Load(song);
   LyricsLoaded();
}

void LyricsWindow::LyricsLoaded()
{
   Clear();

   if (Main::LyricsLoader::Instance().Loaded()) 
   {
      lyrics_.Add(Main::LyricsLoader::Instance().Artist() + " - " + Main::LyricsLoader::Instance().Title());
      lyrics_.Add("");

      if (Main::LyricsBuffer().Size() > 0) 
      {
         for (int i = 0; i < Main::LyricsBuffer().Size(); ++i)
         {
            lyrics_.Add(Main::LyricsBuffer().Get(i));
         }
      }
      else
      {
         lyrics_.Add("No lyrics were found.");
      }
   }
   else
   {
      lyrics_.Add("Loading...");
   }
}

void LyricsWindow::Edit()
{
   int const LyricsWindowId = screen_.GetActiveWindow();
   screen_.SetVisible(LyricsWindowId, false);
}

void LyricsWindow::Redraw()
{
   ScrollTo(0);
   LyricsLoaded();
}

void LyricsWindow::Clear()
{
   lyrics_.Clear();
}

void LyricsWindow::Scroll(int32_t scrollCount)
{
   currentLine_ += scrollCount;
   LimitCurrentSelection();
   ScrollWindow::Scroll(scrollCount);
}

void LyricsWindow::ScrollTo(uint32_t scrollLine)
{
   int64_t oldSelection = currentLine_;
   currentLine_    = (static_cast<int64_t>(scrollLine));
   LimitCurrentSelection();

   ScrollWindow::ScrollTo(scrollLine);
}

/* vim: set sw=3 ts=3: */
