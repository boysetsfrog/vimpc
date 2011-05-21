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

   screen.cpp - ncurses window management
   */

#include "screen.hpp"

#include "config.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <signal.h>

#include "buffers.hpp"
#include "colour.hpp"
#include "settings.hpp"
#include "window/browsewindow.hpp"
#include "window/console.hpp"
#include "window/error.hpp"
#include "window/help.hpp"
#include "window/librarywindow.hpp"
#include "window/playlistwindow.hpp"

using namespace Ui;

bool WindowResized;

extern "C" void ResizeHandler(int);

Screen::Screen(Main::Settings const & settings, Mpc::Client & client, Ui::Search const & search) :
   window_          (Playlist),
   statusWindow_    (NULL),
   tabWindow_       (NULL),
   commandWindow_   (NULL),
   started_         (false),
   maxRows_         (0),
   maxColumns_      (0),
   settings_        (settings)
{
   STATIC_ASSERT((sizeof(mainWindows_)/sizeof(Window *)) == MainWindowCount);

   // ncurses initialisation
   initscr();
   halfdelay(1);
   noecho();

   getmaxyx(stdscr, maxRows_, maxColumns_);
   maxRows_   -= 3; //Status and Mode window use last 2 rows, tabline uses top

   //handle a resize signal
   signal(SIGWINCH, ResizeHandler);

   Ui::Colour::InitialiseColours();

   // Create all the windows
   mainWindows_[Help]     = new Ui::HelpWindow    (*this);
   mainWindows_[Console]  = new Ui::ConsoleWindow (*this);
   mainWindows_[Library]  = new Ui::LibraryWindow (settings, *this, client, search); 
   mainWindows_[Browse]   = new Ui::BrowseWindow  (settings, *this, client, search); 
   mainWindows_[Playlist] = new Ui::PlaylistWindow(settings, *this, client, search);
   statusWindow_          = newwin(1, maxColumns_, maxRows_ + 1, 0);
   tabWindow_             = newwin(1, maxColumns_, 0, 0);

   // Mark them all as visible
   for (uint32_t i = 0; i < MainWindowCount; ++i)
   {
      visibleWindows_.push_back(static_cast<MainWindow>(i));
   }

   // Commands must be read through a window that is always visible
   commandWindow_         = statusWindow_;    
   keypad(commandWindow_, true);

   // Window setup
   mainWindows_[Console]->SetAutoScroll(true);
   SetStatusLine("%s", "");

   ENSURE(WindowsAreInitialised() == true);

   SetVisible(Console, false);
}

Screen::~Screen()
{
   delwin(tabWindow_);
   delwin(statusWindow_);

   for (int i = 0; i < MainWindowCount; ++i)
   {
      delete mainWindows_[i];
   }

   endwin();
}


/* static */ Screen::MainWindow Screen::GetWindowFromName(std::string const & name)
{
   typedef std::map<std::string, MainWindow> WindowTable;
   static WindowTable windowTable;

   MainWindow window = Playlist;

   if (windowTable.size() != MainWindowCount)
   {
      // Names for each of the windows
      windowTable["help"]     = Help;
      windowTable["console"]  = Console;
      windowTable["library"]  = Library;
      windowTable["browse"]   = Browse;
      windowTable["playlist"] = Playlist;
   }

   WindowTable::const_iterator it = windowTable.find(name);

   if (it != windowTable.end())
   {
      window = it->second;
   }
   
   ENSURE(windowTable.size() == MainWindowCount);

   return window;
}


/* static */ std::string Screen::GetNameFromWindow(Screen::MainWindow window)
{
   typedef std::map<MainWindow, std::string> WindowTable;
   static WindowTable windowTable;

   std::string name = "unknown";

   if (windowTable.size() != MainWindowCount)
   {
      // Values for each of the windows
      windowTable[Help]     = "help";
      windowTable[Console]  = "console";
      windowTable[Library]  = "library";
      windowTable[Browse]   = "browse";
      windowTable[Playlist] = "playlist";
   }

   WindowTable::const_iterator it = windowTable.find(window);

   if (it != windowTable.end())
   {
      name = it->second;
   }

   ENSURE(windowTable.size() == MainWindowCount);
   
   return name;
}


void Screen::Start()
{
   REQUIRE(started_ == false);

   if (started_ == false)
   {
      started_ = true;
      SetActiveAndVisible(settings_.Window());
      wrefresh(statusWindow_);
   }

   ENSURE(started_ == true);
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

   wattron(statusWindow_, COLOR_PAIR(STATUSLINECOLOUR) | A_BOLD);
   wmove(statusWindow_, 0, 0);

   va_list args;
   va_start(args, fmt);
   vw_printw(statusWindow_, fmt, args);
   va_end(args);

   wrefresh(statusWindow_);
}

void Screen::MoveSetStatus(uint16_t x, char const * const fmt, ...) const
{
   wattron(statusWindow_, COLOR_PAIR(STATUSLINECOLOUR) | A_BOLD);
   wmove(statusWindow_, 0, x);

   va_list args;
   va_start(args, fmt);
   vw_printw(statusWindow_, fmt, args);
   va_end(args);

   wrefresh(statusWindow_);
}


void Screen::AlignTo(Location location, uint32_t line)
{
   int scrollLine = (line != 0) ? (line - 1) : ActiveWindow().CurrentLine();
   int selection  = scrollLine;

   if (location == Bottom)
   {
      scrollLine += (1 - ((MaxRows() + 1) / 2));
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

   scroll[Top]      = 0;
   scroll[Bottom]   = ActiveWindow().ContentSize(); 
   scroll[Current]  = ActiveWindow().Current(); 
   scroll[Specific] = line - 1;

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

void Screen::Redraw(MainWindow window) const
{
   mainWindows_[window]->Redraw();
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
         wclear(mainWindows_[i]->N_WINDOW());
         mainWindows_[i]->Resize(maxRows_, maxColumns_);
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


Ui::ScrollWindow & Screen::ActiveWindow() const
{
   return assert_reference(mainWindows_[window_]);
}

Screen::MainWindow Screen::GetActiveWindow() const
{
   return window_;
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

   SetActiveWindow(static_cast<MainWindow>(window));
}

void Screen::SetVisible(MainWindow window, bool visible)
{
   //! \todo Handle the case when there is no visible tabs left and clear the mainwindow
   bool found = false;

   if ((window == window_) && (visible == false))
   {
      SetActiveWindow(Ui::Screen::Next);
   }

   if (window < MainWindowCount)
   {
      for (std::vector<MainWindow>::iterator it = visibleWindows_.begin(); ((it != visibleWindows_.end()) && (found == false)); ++it)
      {
         if ((*it) == window)
         {
            found = true;

            if (visible == false)
            {
               visibleWindows_.erase(it);
            }
         }
      }

      if ((found == false) && (visible == true))
      {
         visibleWindows_.push_back(window);
      }
   }
}

void Screen::SetActiveAndVisible(MainWindow window)
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

void Screen::MoveWindow(uint32_t position)
{
   MoveWindow(window_, position);
}

void Screen::MoveWindow(MainWindow window, uint32_t position)
{
   bool found = false;

   uint32_t pos = 0;

   if (position >= visibleWindows_.size())
   {
      position = visibleWindows_.size() - 1;
   }

   for (std::vector<MainWindow>::iterator it = visibleWindows_.begin(); ((it != visibleWindows_.end()) && (found == false)); ++it)
   {
      if ((*it) == window)
      {
         found = true;
         visibleWindows_.erase(it);
      }
   }

   std::vector<MainWindow>::iterator it;

   for (it = visibleWindows_.begin(); ((it != visibleWindows_.end()) && (pos < position)); ++it, ++pos) { }

   if (pos == position)
   {
      visibleWindows_.insert(it, window);
   }
}


void Screen::ClearStatus() const
{
   std::string const BlankLine(maxColumns_, ' ');

   wattron(statusWindow_,   COLOR_PAIR(STATUSLINECOLOUR));
   mvwprintw(statusWindow_, 0, 0, BlankLine.c_str());
}

void Screen::UpdateTabWindow() const
{
   std::string const BlankLine(maxColumns_, ' ');

   werase(tabWindow_);
   wattron(tabWindow_, COLOR_PAIR(DEFAULTONBLUE));
   mvwprintw(tabWindow_, 0, 0, BlankLine.c_str());

   std::string name   = "";
   uint32_t    length = 0;
   uint32_t    count  = 0;

   for (std::vector<MainWindow>::const_iterator it = visibleWindows_.begin(); (it != visibleWindows_.end()); ++it)
   {
      wattron(tabWindow_, COLOR_PAIR(DEFAULTONBLUE) | A_UNDERLINE);
      name = GetNameFromWindow(static_cast<MainWindow>(*it));

      if (*it == window_)
      {
         wattron(tabWindow_, COLOR_PAIR(DEFAULTONBLUE) | A_REVERSE | A_BOLD);
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

   wattroff(tabWindow_, A_UNDERLINE);
   wrefresh(tabWindow_);
}


bool Screen::WindowsAreInitialised()
{
   bool result = true;

   for (int i = 0; ((i < MainWindowCount) && (result == true)); ++i)
   {
      result = (mainWindows_[i] != NULL);
   }

   return result;
}


void ResizeHandler(UNUSED int i)
{
   WindowResized = true;
}
