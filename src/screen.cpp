#include "screen.hpp"

#include "console.hpp"
#include "playlist.hpp"
#include "settings.hpp"

#include <iostream>

using namespace Ui;

Screen::Screen(Mpc::Client & client, Main::Settings const & settings) :
   window_          (Playlist),
   statusWindow_    (NULL),
   commandWindow_   (NULL),
   settings_        (settings),
   started_         (false),
   maxRows_         (0),
   maxColumns_      (0)
{
   STATIC_ASSERT((sizeof(mainWindows_)/sizeof(Window *)) == MainWindowCount);

   // \todo for some reason when i run this
   // using gnuscreen the cursor doesn't appear?
   initscr();
   start_color();
   use_default_colors();
   cbreak();
   noecho();

   getmaxyx(stdscr, maxRows_, maxColumns_);

   //Status and Mode window use last 2 rows
   maxRows_ -= 2;

   //Windows
   playlistWindow_        = new Ui::PlaylistWindow(*this, client);
   consoleWindow_         = new Ui::ConsoleWindow (*this);
   libraryWindow_         = new Ui::ConsoleWindow (*this); // \todo
   helpWindow_            = new Ui::ConsoleWindow (*this); // \todo
   statusWindow_          = newwin(0, maxColumns_, maxRows_, 0);

   //Commands must be read through a window that is always visible
   commandWindow_         = statusWindow_;    

   //
   mainWindows_[Playlist] = playlistWindow_;
   mainWindows_[Console]  = consoleWindow_;
   mainWindows_[Library]  = libraryWindow_;
   mainWindows_[Help]     = helpWindow_;

   keypad(commandWindow_, true);
   curs_set(0);

   consoleWindow_->SetAutoScroll(true);
   SetStatusLine("%s", "");

   ENSURE(WindowsAreInitialised() == true);
}

Screen::~Screen()
{
   for (int i = 0; i < MainWindowCount; ++i)
   {
      delete mainWindows_[i];
   }

   delwin(statusWindow_);
   delwin(commandWindow_);
   endwin();
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
   return (new ModeWindow(*this));
}

void Screen::SetStatusLine(char const * const fmt, ...)
{
   // \todo needs to be a different colour to the selection
   std::string const BlankLine(maxColumns_, ' ');

   wattron(statusWindow_,   A_REVERSE | A_BOLD);
   mvwprintw(statusWindow_, 0, 0, BlankLine.c_str());
   wmove(statusWindow_,     0, 0);

   va_list args;
   va_start(args, fmt);
   vw_printw(statusWindow_, fmt, args);
   va_end(args);

   wrefresh(statusWindow_);
}


void Screen::Confirm()
{
   mainWindows_[window_]->Confirm();
}

void Screen::Scroll(int32_t count)
{
   mainWindows_[window_]->Scroll(count);
}

void Screen::ScrollTo(uint32_t line)
{
   mainWindows_[window_]->ScrollTo(line);
}

void Screen::Update() const
{
   if ((started_ == true) && (mainWindows_[window_] != NULL))
   {
      Ui::Window & mainWindow = DBC::Reference<Ui::Window>(mainWindows_[window_]);

      mainWindow.Erase();

      for (uint32_t i = 0; (i < maxRows_); ++i)
      {
         mainWindow.Print(i);
      }

      mainWindow.Refresh();
      wnoutrefresh(statusWindow_);
      doupdate();
   }
}

void Screen::Search(std::string const & searchString) const
{
   mainWindows_[window_]->Search(searchString);
}

void Screen::Clear()
{
   consoleWindow_->Clear();

   if (window_ == Console)
   {
      Update();
   }
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
   return wgetch(commandWindow_);
}


void Screen::SetActiveWindow(MainWindow window)
{
   window_ = window;
   Update();
}

Ui::ConsoleWindow & Screen::ConsoleWindow() const
{ 
   return DBC::Reference<Ui::ConsoleWindow>(consoleWindow_);
}

Ui::PlaylistWindow & Screen::PlaylistWindow() const
{ 
   return DBC::Reference<Ui::PlaylistWindow>(playlistWindow_);
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

