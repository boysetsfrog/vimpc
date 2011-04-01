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

#include "colour.hpp"
#include "settings.hpp"
#include "window/console.hpp"
#include "window/error.hpp"
#include "window/help.hpp"
#include "window/librarywindow.hpp"
#include "window/playlistwindow.hpp"

using namespace Ui;

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
   maxRows_ -= 3; //Status and Mode window use last 2 rows, tabline uses top

   Ui::Colour::InitialiseColours();

   // Create all the windows
   mainWindows_[Help]     = new Ui::HelpWindow    (*this);
   mainWindows_[Console]  = new Ui::ConsoleWindow (*this);
   mainWindows_[Library]  = new Ui::LibraryWindow (settings, *this, client, search); 
   mainWindows_[Playlist] = new Ui::PlaylistWindow(settings, *this, client, search);
   statusWindow_          = newwin(1, maxColumns_, maxRows_ + 1, 0);
   tabWindow_             = newwin(1, maxColumns_, 0, 0);

   // Commands must be read through a window that is always visible
   commandWindow_         = statusWindow_;    
   keypad(commandWindow_, true);

   // Window setup
   mainWindows_[Console]->SetAutoScroll(true);
   SetStatusLine("%s", "");

   ENSURE(WindowsAreInitialised() == true);
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
      SetActiveWindow(settings_.Window());
      wrefresh(statusWindow_);
   }

   ENSURE(started_ == true);
}

ModeWindow * Screen::CreateModeWindow()
{
   return (new ModeWindow());
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
   Ui::Console::Instance().Clear();

   if (window_ == Console)
   {
      Update();
   }
}

void Screen::Update() const
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

   int32_t input = wgetch(commandWindow_);

   if (input != ERR)
   {
      // \todo make own function
      errorWindow.ClearError();
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

void Screen::SetActiveWindow(MainWindow window)
{
   if (window < MainWindowCount)
   {
      window_ = window;
   }

   Update();
}

void Screen::SetActiveWindow(Skip skip)
{
   int32_t window = window_ + ((skip == Next) ? 1 : -1);

   if (window >= MainWindowCount)
   {
      window -= MainWindowCount;
   }
   else if (window < 0)
   {
      window += MainWindowCount;
   }

   SetActiveWindow(static_cast<MainWindow>(window));
}


void Screen::ClearStatus() const
{
   static std::string const BlankLine(maxColumns_, ' ');

   wattron(statusWindow_,   COLOR_PAIR(STATUSLINECOLOUR));
   mvwprintw(statusWindow_, 0, 0, BlankLine.c_str());
}

void Screen::UpdateTabWindow() const
{
   static std::string const BlankLine(maxColumns_, ' ');

   werase(tabWindow_);
   wattron(tabWindow_, COLOR_PAIR(DEFAULTONBLUE));
   mvwprintw(tabWindow_, 0, 0, BlankLine.c_str());

   std::string name   = "";
   uint32_t    length = 0;

   for (int i = 0; i < MainWindowCount; ++i)
   {
      wattron(tabWindow_, COLOR_PAIR(DEFAULTONBLUE) | A_UNDERLINE);
      name = GetNameFromWindow(static_cast<MainWindow>(i));

      if (i == window_)
      {
         wattron(tabWindow_, COLOR_PAIR(DEFAULTONBLUE) | A_REVERSE | A_BOLD);
      }

      wmove(tabWindow_, 0, length);

      if (settings_.WindowNumbers() == true)
      {
         waddstr(tabWindow_, "[");
         wattron(tabWindow_, A_BOLD);
         wprintw(tabWindow_, "%d", i + 1);
      
         if (i != window_)
         {
            wattroff(tabWindow_, A_BOLD);
         }

         waddstr(tabWindow_, "]");

         length += 3;
      }
      
      wprintw(tabWindow_, " %s ", name.c_str());

      wattroff(tabWindow_, A_REVERSE | A_BOLD);

      length += name.size() + 2;
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

