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

   screen.cpp - ncurses window management
   */

#include "screen.hpp"

#include "config.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <signal.h>

#include "algorithm.hpp"
#include "buffers.hpp"
#include "colour.hpp"
#include "mpdclient.hpp"
#include "settings.hpp"

#include "window/browsewindow.hpp"
#include "window/console.hpp"
#include "window/error.hpp"
#include "window/help.hpp"
#include "window/librarywindow.hpp"
#include "window/listwindow.hpp"
#include "window/outputwindow.hpp"
#include "window/playlistwindow.hpp"
#include "window/songwindow.hpp"

using namespace Ui;

bool WindowResized;

extern "C" void ResizeHandler(int);

Screen::Screen(Main::Settings & settings, Mpc::Client & client, Ui::Search const & search) :
   window_          (Playlist),
   statusWindow_    (NULL),
   tabWindow_       (NULL),
   commandWindow_   (NULL),
   started_         (false),
   maxRows_         (0),
   maxColumns_      (0),
   settings_        (settings),
   client_          (client),
   search_          (search)
{
   // ncurses initialisation
   initscr();

   if (has_colors() == false)
   {
      settings.Set("nocolour");
   }
   else
   {
      if (Ui::Colour::InitialiseColours() == false)
      {
         settings.Set("nocolour");
      }
   }

   halfdelay(1);
   noecho();

#ifdef HAVE_MOUSE_SUPPORT
   mousemask(ALL_MOUSE_EVENTS, NULL);
#endif

   getmaxyx(stdscr, maxRows_, maxColumns_);
   maxRows_   -= 3; //Status and Mode window use last 2 rows, tabline uses top

   //handle a resize signal
   signal(SIGWINCH, ResizeHandler);

   // Create all the windows
   mainWindows_[Help]     = new Ui::HelpWindow    (settings, *this);
   mainWindows_[Console]  = new Ui::ConsoleWindow (*this);
   mainWindows_[Outputs]  = new Ui::OutputWindow  (settings, *this, client, search);
   mainWindows_[Library]  = new Ui::LibraryWindow (settings, *this, client, search);
   mainWindows_[Browse]   = new Ui::BrowseWindow  (settings, *this, client, search);

#if LIBMPDCLIENT_CHECK_VERSION(2,5,0)
   mainWindows_[Lists]    = new Ui::ListWindow    (settings, *this, client, search);
#else
   mainWindows_[Lists]    = NULL;
#endif

   mainWindows_[Playlist] = new Ui::PlaylistWindow(settings, *this, client, search);
   statusWindow_          = newwin(1, maxColumns_, maxRows_ + 1, 0);
   tabWindow_             = newwin(1, maxColumns_, 0, 0);

   // Mark them all as visible
   for (uint32_t i = 0; i < MainWindowCount; ++i)
   {
      if (mainWindows_[i] != NULL)
      {
         visibleWindows_.push_back(static_cast<int32_t>(i));
      }
   }

   // Commands must be read through a window that is always visible
   commandWindow_         = statusWindow_;
   keypad(commandWindow_, true);

   // Window setup
   mainWindows_[Console]->SetAutoScroll(true);
   SetStatusLine("%s", "");

   SetVisible(Console, false);
}

Screen::~Screen()
{
   delwin(tabWindow_);
   delwin(statusWindow_);

   for (WindowMap::iterator it = mainWindows_.begin(); it != mainWindows_.end(); ++it)
   {
      delete it->second;
   }

   endwin();
}


int32_t Screen::GetWindowFromName(std::string const & name) const
{
   WindowMap::const_iterator it = mainWindows_.begin();

   int32_t window = Unknown;

   for (; it != mainWindows_.end(); ++it)
   {
      if ((it->second != NULL) && (Algorithm::iequals(name, it->second->Name()) == true))
      {
         window = it->first;
      }
   }

   return window;
}


std::string Screen::GetNameFromWindow(int32_t window) const
{
   WindowMap::const_iterator it = mainWindows_.find(window);

   std::string name = "Unknown";

   if (it != mainWindows_.end())
   {
      name = it->second->Name();
   }

   return name;
}


void Screen::Start()
{
   REQUIRE(started_ == false);

   if (started_ == false)
   {
      started_ = true;
      SetActiveAndVisible(GetWindowFromName(settings_.Window()));
      wrefresh(statusWindow_);
   }

   ENSURE(started_ == true);
}


Ui::SongWindow * Screen::CreateWindow(std::string const & name)
{
   int32_t id = static_cast<int32_t>(Dynamic);

   while (mainWindows_.find(id) != mainWindows_.end())
   {
      ++id;
   }

   Ui::SongWindow * window = new SongWindow(settings_, *this, client_, search_, name);
   mainWindows_[id]        = window;

   visibleWindows_.push_back(id);

   window_ = id;

   return window;
}


ModeWindow * Screen::CreateModeWindow()
{
   ModeWindow * window = new ModeWindow();
   modeWindows_.push_back(window);
   return window;
}

void Screen::DeleteModeWindow(ModeWindow * window)
{
   bool found = false;

   for (std::vector<ModeWindow *>::iterator it = modeWindows_.begin(); ((it != modeWindows_.end()) && (found != true)); )
   {
      if (*it == window)
      {
         found = true;
         delete *it;
         it = modeWindows_.erase(it);
      }
      else
      {
         ++it;
      }
   }
}


void Screen::SetStatusLine(char const * const fmt, ...) const
{
   ClearStatus();

   if (settings_.ColourEnabled() == true)
   {
      wattron(statusWindow_, COLOR_PAIR(Colour::StatusLine));
   }
   else
   {
      wattron(statusWindow_, A_REVERSE);
   }

   wattron(statusWindow_, A_BOLD);
   wmove(statusWindow_, 0, 0);

   va_list args;
   va_start(args, fmt);
   vw_printw(statusWindow_, fmt, args);
   va_end(args);

   if (settings_.ColourEnabled() == true)
   {
      wattroff(statusWindow_, COLOR_PAIR(Colour::StatusLine));
   }
   else
   {
      wattroff(statusWindow_, A_REVERSE);
   }

   wrefresh(statusWindow_);
}

void Screen::MoveSetStatus(uint16_t x, char const * const fmt, ...) const
{
   if (settings_.ColourEnabled() == true)
   {
      wattron(statusWindow_, COLOR_PAIR(Colour::StatusLine));
   }
   else
   {
      wattron(statusWindow_, A_REVERSE);
   }

   wattron(statusWindow_, A_BOLD);
   wmove(statusWindow_, 0, x);

   va_list args;
   va_start(args, fmt);
   vw_printw(statusWindow_, fmt, args);
   va_end(args);

   if (settings_.ColourEnabled() == true)
   {
      wattroff(statusWindow_, COLOR_PAIR(Colour::StatusLine));
   }
   else
   {
      wattroff(statusWindow_, A_REVERSE);
   }

   wrefresh(statusWindow_);
}


void Screen::Align(Direction direction, uint32_t line)
{
   int selection  = ActiveWindow().CurrentLine();
   int min        = ActiveWindow().FirstLine();
   int max        = MaxRows();
   int scrollLine = ActiveWindow().CurrentLine();

   if (direction == Up)
   {
      if (selection == (min + max - 1))
         selection--;

      ActiveWindow().ScrollWindow::Scroll(-1);
   }
   else if (direction == Down)
   {
      if (selection == min)
         selection++;

      ActiveWindow().ScrollWindow::Scroll(1);
   }

   // Uses the select window scroll to set the current line properly
   ScrollTo(selection);
}

void Screen::AlignTo(Location location, uint32_t line)
{
   int scrollLine = (line != 0) ? (line - 1) : ActiveWindow().CurrentLine();
   int selection  = scrollLine;

   if (location == Bottom)
   {
      scrollLine += (1 - ((MaxRows()) / 2));
   }
   else if (location == Top)
   {
      scrollLine += ((MaxRows() + 1) / 2);
   }

   // Uses the scroll window scroll to set the right area of the screen
   ActiveWindow().ScrollWindow::ScrollTo(scrollLine);

   // Uses the select window scroll to set the current line properly
   ScrollTo(selection);
}

void Screen::Select(ScrollWindow::Position position, uint32_t count)
{
   ActiveWindow().Select(position, count);
}

void Screen::Scroll(int32_t count)
{
   ActiveWindow().Scroll(count);
}

void Screen::Scroll(Size size, Direction direction, uint32_t count)
{
   int32_t scrollCount = count;

   if (size == Page)
   {
      scrollCount *= (MaxRows() / 2);
   }

   if (direction == Up)
   {
      scrollCount *= -1;
   }

   Scroll(scrollCount);
}

void Screen::ScrollTo(uint32_t line)
{
   ActiveWindow().ScrollTo(line);
}

void Screen::ScrollTo(Location location, uint32_t line)
{
   uint32_t scroll[LocationCount] = { 0, 0, 0, 0 };

   scroll[Top]          = 0;
   scroll[Bottom]       = ActiveWindow().ContentSize();
   scroll[Current]      = ActiveWindow().Current() + line;
   scroll[PlaylistNext] = ActiveWindow().Playlist(1);
   scroll[PlaylistPrev] = ActiveWindow().Playlist(-1);
   scroll[Specific]     = line - 1;

   ScrollTo(scroll[location]);
}


void Screen::Clear()
{
   Main::Console().Clear();

   if (window_ == Console)
   {
      Update();
   }
}

void Screen::Update()
{
   if ((started_ == true) && (mainWindows_[window_] != NULL))
   {
      ActiveWindow().Erase();

      UpdateTabWindow();

      for (uint32_t i = 0; (i < maxRows_); ++i)
      {
         ActiveWindow().Print(i);
      }

      ActiveWindow().Refresh();
      doupdate();
   }
}

void Screen::Redraw() const
{
   Redraw(window_);
}

void Screen::Redraw(int32_t window) const
{
   WindowMap::const_iterator it = mainWindows_.find(window);

   if ((it != mainWindows_.end()) && (it->second != NULL))
   {
      (it->second)->Redraw();
   }
}

bool Screen::Resize(bool forceResize)
{
   bool WasWindowResized = false;

   if ((WindowResized == true) || (forceResize == true))
   {
      WasWindowResized = true;
      WindowResized    = false;

      // Get the window size with thanks to irssi
#ifdef TIOCGWINSZ
      struct winsize windowSize;

      if ((ioctl(0, TIOCGWINSZ, &windowSize) >= 0) && (windowSize.ws_row > 0 && windowSize.ws_col > 0))
      {
         maxRows_    = windowSize.ws_row;
         maxColumns_ = windowSize.ws_col;
      }
#else
      endwin();
      refresh();
      getmaxyx(stdscr, maxRows_, maxColumns_);
#endif

      resizeterm(maxRows_, maxColumns_);
      wresize(stdscr, maxRows_, maxColumns_);
      refresh();

      maxRows_ -= 3;

      wresize(statusWindow_, 1, maxColumns_);
      wresize(tabWindow_,    1, maxColumns_);
      mvwin(statusWindow_, maxRows_ + 1, 0);

      for (std::vector<ModeWindow *>::iterator it = modeWindows_.begin(); (it != modeWindows_.end()); ++it)
      {
         (*it)->Resize(1, maxColumns_);
         (*it)->Move(maxRows_ + 2, 0);
      }

      for (int i = 0; (i < MainWindowCount); ++i)
      {
         if (mainWindows_[i] != NULL)
         {
            wclear(mainWindows_[i]->N_WINDOW());
            mainWindows_[i]->Resize(maxRows_, maxColumns_);
         }
      }
   }

   return WasWindowResized;
}


uint32_t Screen::MaxRows() const
{
   return maxRows_;
}

uint32_t Screen::MaxColumns() const
{
   return maxColumns_;
}

uint32_t Screen::WaitForInput() const
{
   // \todo this doesn't seem to work if constructed
   // when the screen is constructed, find out why
   Ui::ErrorWindow & errorWindow(Ui::ErrorWindow::Instance());

   if (errorWindow.HasError() == true)
   {
      errorWindow.Print(0);
   }

   halfdelay(1);

   int32_t input = wgetch(commandWindow_);

   if (input != ERR)
   {
      // \todo make own function
      errorWindow.ClearError();
   }

   if (input == 27)
   {
      cbreak();

      int escapeChar = wgetch(commandWindow_);

      if ((escapeChar != ERR) && (escapeChar != 27))
      {
         input = escapeChar | (1 << 31);
      }
   }

   return input;
}

void Screen::ClearInput() const
{
   halfdelay(1);

   int32_t input = ERR;

   do
   {
      input = wgetch(commandWindow_);
   } while (input != ERR);
}

void Screen::HandleMouseEvent()
{
#ifdef HAVE_MOUSE_SUPPORT
   MEVENT event;

   if (getmouse(&event) == OK)
   {
      if (event.y == 0)
      {
         if (((event.bstate & BUTTON1_CLICKED) == BUTTON1_CLICKED) || ((event.bstate & BUTTON1_DOUBLE_CLICKED) == BUTTON1_DOUBLE_CLICKED))
         {
            int32_t x = 0;
            for (std::vector<int32_t>::const_iterator it = visibleWindows_.begin(); (it != visibleWindows_.end()); ++it)
            {
               std::string name = GetNameFromWindow(static_cast<int32_t>(*it));
               x += name.length() + 2;

               if (event.x < x)
               {
                  SetActiveAndVisible(GetWindowFromName(name));
                  break;
               }
            }
         }
      }
      else if ((event.y > 0) && (event.y <= static_cast<int32_t>(MaxRows())))
      {
         if (((event.bstate & BUTTON1_CLICKED) == BUTTON1_CLICKED) || ((event.bstate & BUTTON1_DOUBLE_CLICKED) == BUTTON1_DOUBLE_CLICKED))
         {
            ActiveWindow().ScrollTo(ActiveWindow().FirstLine() + event.y - 1);
         }

         if ((event.bstate & BUTTON1_DOUBLE_CLICKED) == BUTTON1_DOUBLE_CLICKED)
         {
            ActiveWindow().Confirm();
         }
      }
   }
#endif
}


int32_t Screen::GetActiveWindow() const
{
   return window_;
}

Ui::ScrollWindow & Screen::ActiveWindow() const
{
   WindowMap::const_iterator it = mainWindows_.find(window_);
   return assert_reference(it->second);
}

Ui::ScrollWindow & Screen::Window(uint32_t window) const
{
   WindowMap::const_iterator it = mainWindows_.find(window);
   return assert_reference(it->second);
}


void Screen::SetActiveWindow(uint32_t window)
{
   if ((window < visibleWindows_.size()))
   {
      window_ = visibleWindows_.at(window);
   }

   Update();
}

void Screen::SetActiveWindow(Skip skip)
{
   int32_t currentIndex = -1;

   for (uint32_t i = 0; ((i < visibleWindows_.size()) && (currentIndex == -1)); ++i)
   {
      if (visibleWindows_.at(i) == window_)
      {
         currentIndex = i;
      }
   }

   int32_t window = currentIndex + ((skip == Next) ? 1 : -1);

   if (window >= static_cast<int32_t>(visibleWindows_.size()))
   {
      window -= visibleWindows_.size();
   }
   else if (window < 0)
   {
      window += visibleWindows_.size();
   }

   SetActiveWindow(static_cast<int32_t>(window));
}

void Screen::SetVisible(int32_t window, bool visible)
{
   if (mainWindows_[window] != NULL)
   {
      //! \todo Handle the case when there is no visible tabs left and clear the mainwindow
      bool found = false;

      if ((window == window_) && (visible == false) && (visibleWindows_.back() == window))
      {
         SetActiveWindow(Ui::Screen::Previous);
      }
      else if ((window == window_) && (visible == false))
      {
         SetActiveWindow(Ui::Screen::Next);
      }

      for (std::vector<int32_t>::iterator it = visibleWindows_.begin(); ((it != visibleWindows_.end()) && (found == false)); ++it)
      {
         if ((*it) == window)
         {
            found = true;

            if (visible == false)
            {
               visibleWindows_.erase(it);

               if (window >= Dynamic)
               {
                  WindowMap::iterator jt = mainWindows_.find(window);
                  delete jt->second;
                  mainWindows_.erase(jt);
               }
            }
         }
      }

      if ((found == false) && (visible == true))
      {
         visibleWindows_.push_back(window);
      }
   }
}

void Screen::SetActiveAndVisible(int32_t window)
{
   if (mainWindows_[window] != NULL)
   {
      SetVisible(window, true);

      for (uint32_t i = 0; i < visibleWindows_.size(); ++i)
      {
         if (visibleWindows_.at(i) == window)
         {
            SetActiveWindow(i);
         }
      }
   }
}

void Screen::MoveWindow(uint32_t position)
{
   MoveWindow(window_, position);
}

void Screen::MoveWindow(int32_t window, uint32_t position)
{
   if (mainWindows_[window] != NULL)
   {
      bool found = false;

      uint32_t pos = 0;

      if (position >= visibleWindows_.size())
      {
         position = visibleWindows_.size() - 1;
      }

      for (std::vector<int32_t>::iterator it = visibleWindows_.begin(); ((it != visibleWindows_.end()) && (found == false)); ++it)
      {
         if ((*it) == window)
         {
            found = true;
            visibleWindows_.erase(it);
         }
      }

      std::vector<int32_t>::iterator it;

      for (it = visibleWindows_.begin(); ((it != visibleWindows_.end()) && (pos < position)); ++it, ++pos) { }

      if (pos == position)
      {
         visibleWindows_.insert(it, window);
      }
   }
}


void Screen::ClearStatus() const
{
   std::string BlankLine(maxColumns_, ' ');

   werase(statusWindow_);

   if (settings_.ColourEnabled() == true)
   {
      wattron(statusWindow_, COLOR_PAIR(Colour::StatusLine));
   }
   else
   {
      wattron(statusWindow_, A_REVERSE);
   }

   mvwprintw(statusWindow_, 0, 0, BlankLine.c_str());

   if (settings_.ColourEnabled() == true)
   {
      wattroff(statusWindow_, COLOR_PAIR(Colour::StatusLine));
   }
   else
   {
      wattroff(statusWindow_, A_REVERSE);
   }
}

void Screen::UpdateTabWindow() const
{
   std::string const BlankLine(maxColumns_, ' ');

   werase(tabWindow_);

   if (settings_.ColourEnabled() == true)
   {
      wattron(tabWindow_, COLOR_PAIR(DEFAULTONBLUE));
   }

   mvwprintw(tabWindow_, 0, 0, BlankLine.c_str());

   std::string name   = "";
   uint32_t    length = 0;
   uint32_t    count  = 0;

   for (std::vector<int32_t>::const_iterator it = visibleWindows_.begin(); (it != visibleWindows_.end()); ++it)
   {
      if (settings_.ColourEnabled() == true)
      {
         wattron(tabWindow_, COLOR_PAIR(DEFAULTONBLUE));
      }

      wattron(tabWindow_, A_UNDERLINE);
      name = GetNameFromWindow(static_cast<int32_t>(*it));

      if (*it == window_)
      {
         if (settings_.ColourEnabled() == true)
         {
            wattron(tabWindow_, COLOR_PAIR(DEFAULTONBLUE));
         }

         wattron(tabWindow_, A_REVERSE | A_BOLD);
      }

      wmove(tabWindow_, 0, length);

      if (settings_.WindowNumbers() == true)
      {
         waddstr(tabWindow_, "[");
         wattron(tabWindow_, A_BOLD);
         wprintw(tabWindow_, "%d", count + 1);

         if (*it != window_)
         {
            wattroff(tabWindow_, A_BOLD);
         }

         waddstr(tabWindow_, "]");

         length += 3;
      }

      wprintw(tabWindow_, " %s ", name.c_str());

      wattroff(tabWindow_, A_REVERSE | A_BOLD);

      length += name.size() + 2;
      ++count;
   }

   if (settings_.ColourEnabled() == true)
   {
      wattroff(tabWindow_, COLOR_PAIR(DEFAULTONBLUE));
   }

   wattroff(tabWindow_, A_UNDERLINE);
   wrefresh(tabWindow_);
}


void ResizeHandler(int i)
{
   WindowResized = true;
}
