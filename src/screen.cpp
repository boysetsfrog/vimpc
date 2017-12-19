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

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <csignal>
#include <list>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

#include "algorithm.hpp"
#include "buffers.hpp"
#include "clientstate.hpp"
#include "mpdclient.hpp"
#include "settings.hpp"
#include "song.hpp"
#include "songsorter.hpp"
#include "vimpc.hpp"

#include "window/browsewindow.hpp"
#include "window/console.hpp"
#include "window/debug.hpp"
#include "window/directorywindow.hpp"
#include "window/error.hpp"
#include "window/help.hpp"
#include "window/infowindow.hpp"
#include "window/librarywindow.hpp"
#include "window/listwindow.hpp"
#include "window/outputwindow.hpp"
#include "window/playlistwindow.hpp"
#include "window/result.hpp"
#include "window/songwindow.hpp"
#include "window/windowselector.hpp"

#ifdef LYRICS_SUPPORT
#include "window/lyricswindow.hpp"
#endif

#if (NCURSES_MOUSE_VERSION <= 1)
#ifndef BUTTON5_PRESSED
#define BUTTON5_PRESSED BUTTON4_PRESSED << 8
#endif
#endif

#define INPUT_SIGINT  3
#define INPUT_SIGSTOP 26
#define INPUT_ESCAPE  27

using namespace Ui;


bool WindowResized = false;
bool Continue      = true;
int32_t RndCount   = 0;

static Atomic(bool)   Running(true);
static RecursiveMutex CursesMutex;

extern "C" void ResizeHandler(int);
extern "C" void ContinueHandler(int);

void RandomCharacterInput();
void QueueInput(WINDOW * inputWindow);


std::string Windows::String(uint32_t position) const
{
   return screen_->GetNameFromWindow(Get(position));
}

std::string Windows::PrintString(uint32_t position) const
{
   std::string const Result = " " + String(position);
   return Result;
}

void RandomCharacterInput()
{
   int rndInput = INPUT_SIGSTOP;

   while ((rndInput == INPUT_SIGSTOP) || //<C-Z>
          (rndInput == INPUT_SIGINT))      //<C-C>
   {
      rndInput = (rand() % (KEY_MAX + 1));
   }

   CursesMutex.lock();
   ungetch(rndInput);
   CursesMutex.unlock();
}

void QueueInput(WINDOW * inputWindow)
{
   CursesMutex.lock();
   keypad(inputWindow, true);
   wtimeout(inputWindow, -1);
   CursesMutex.unlock();

   pollfd fds;
   fds.fd = 1;
   fds.events = POLLIN;

   while (Running == true)
   {
   #ifdef __DEBUG_PRINTS
      if (RndCount > 0) { RandomCharacterInput(); --RndCount; }
      else
      {
   #endif

      if (Continue == true)
      {
         Continue = false;

         CursesMutex.lock();
         raw();
         noecho();
         keypad(inputWindow, true);
         wtimeout(inputWindow, -1);
         CursesMutex.unlock();

         EventData Data;
         Main::Vimpc::CreateEvent(Event::Continue, Data);
      }

      if (poll(&fds, 1, 250) <= 0)
      {
         continue;
      }

   #ifdef __DEBUG_PRINTS
      }
   #endif

      CursesMutex.lock();
      int32_t input = wgetch(inputWindow);
      CursesMutex.unlock();

      if (input != ERR)
      {
         if (input != KEY_RESIZE)
         {
            if (input == INPUT_SIGINT) //<C-C>
            {
               std::raise(SIGINT);
            }
            else if (input == INPUT_SIGSTOP) //<C-Z>
            {
               std::raise(SIGSTOP);
            }
         else
            {
               //if ((input == INPUT_ESCAPE) && (HandleEscape == true))
               if (input == INPUT_ESCAPE)
               {
                  CursesMutex.lock();
                  wtimeout(inputWindow, 0);

                  int escapeChar = wgetch(inputWindow);

                  if ((escapeChar != ERR) && (escapeChar != INPUT_ESCAPE))
                  {
                     input = escapeChar | (1 << 31);
                  }

                  wtimeout(inputWindow, -1);
                  CursesMutex.unlock();
               }

               EventData Data;
               Data.user  = true;
               Data.input = input;
               Main::Vimpc::CreateEvent(Event::Input, Data);
            }
         }
      }
   }
}


Screen::Screen(Main::Settings & settings, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search) :
   window_          (Playlist),
   previous_        (Playlist),
   statusWindow_    (NULL),
   tabWindow_       (NULL),
   progressWindow_  (NULL),
   commandWindow_   (NULL),
   pagerWindow_     (NULL),
   started_         (false),
   pager_           (false),
   progress_        (0),
   maxRows_         (0),
   maxColumns_      (0),
   windows_         (this),
   settings_        (settings),
   client_          (client),
   clientState_     (clientState),
   search_          (search)
{
   // ncurses initialisation
   CursesMutex.lock();
   initscr();
   raw();
   noecho();

   if ((has_colors() == false) || (settings_.colours.InitialiseColours() == false))
   {
      settings_.SetSingleSetting("nocolour");
   }

   maxRows_    = LINES;
   maxColumns_ = COLS;
   mainRows_   = ((maxRows_ - 4) > 0) ? (maxRows_ - 4) : 0;

   // Handler for the resize signal
   signal(SIGWINCH, ResizeHandler);
   signal(SIGCONT,  ContinueHandler);

   // Create paging window to print maps, settings, etc
   pagerWindow_               = new PagerWindow(*this, maxColumns_, 0);
   statusWindow_              = newwin(1, maxColumns_, mainRows_ + 2, 0);
   tabWindow_                 = newwin(1, maxColumns_, 0, 0);
   progressWindow_            = newwin(1, maxColumns_, mainRows_ + 1, 0);
   mainWindow_                = newwin(mainRows_, maxColumns_, 1, 0);

   // Create all the static windows
   mainWindows_[Help]         = new Ui::HelpWindow     (settings, *this, search);
   mainWindows_[DebugConsole] = new Ui::ConsoleWindow  (settings, *this, "debug",   Main::DebugConsole());
   mainWindows_[TestConsole]  = new Ui::ConsoleWindow  (settings, *this, "test",    Main::TestConsole());
   mainWindows_[Console]      = new Ui::ConsoleWindow  (settings, *this, "console", Main::Console());
   mainWindows_[Outputs]      = new Ui::OutputWindow   (settings, *this, Main::Outputs(),   client, search);
   mainWindows_[Library]      = new Ui::LibraryWindow  (settings, *this, Main::Library(),   client, clientState, search);
   mainWindows_[Browse]       = new Ui::BrowseWindow   (settings, *this, Main::Browse(),    client, clientState, search);
   mainWindows_[Directory]    = new Ui::DirectoryWindow(settings, *this, Main::Directory(), client, clientState, search);
   mainWindows_[Lists]        = new Ui::ListWindow     (settings, *this, Main::AllLists(),  client, search);
   mainWindows_[Playlist]     = new Ui::PlaylistWindow (settings, *this, Main::Playlist(),  client, clientState, search);
   mainWindows_[WindowSelect] = new Ui::WindowSelector (settings, *this, windows_, search);

   // Commands must be read through a window that is always visible
   commandWindow_             = newwin(0, 0, 0, 0);

   // Mark every tab as visible initially
   // This means that commands such as tabhide in the config file
   // can still be used in addition to :set windows and :set window
   for (int i = 0; i < static_cast<int>(Unknown); ++i)
   {
      if (mainWindows_[i] != NULL)
      {
         visibleWindows_.push_back(i);

         if ((true)
#ifndef __DEBUG_PRINTS
         && (i != (int) DebugConsole)
#endif
#ifndef TEST_ENABLED
         && (i != (int) TestConsole)
#endif
         )
         {
            windows_.Add(i);
         }
      }
   }

   SetVisible(DebugConsole, false);

   // Force auto scroll on the consoles
   mainWindows_[Console]->SetAutoScroll(true);
   mainWindows_[DebugConsole]->SetAutoScroll(true);

   // Register settings callbacks
   settings_.RegisterCallback(Setting::TabBar,      [this] (bool Value) { OnTabSettingChange(Value); });
   settings_.RegisterCallback(Setting::ProgressBar, [this] (bool Value) { OnProgressSettingChange(Value); });
   settings_.RegisterCallback(Setting::Mouse,       [this] (bool Value) { OnMouseSettingChange(Value); });

   // If mouse support is turned on set it up
   SetupMouse(settings_.Get(Setting::Mouse));
   CursesMutex.unlock();

   // Register events
   Main::Vimpc::EventHandler(Event::Continue,  [this] (EventData const & Data)
   {
      SetupMouse(settings_.Get(Setting::Mouse));
   });

   Main::Vimpc::EventHandler(Event::AllMetaDataReady, [this] (EventData const & Data) { InvalidateAll(); });

   Main::Vimpc::EventHandler(Event::RequirePassword,  [this] (EventData const & Data)
   {
      PromptForPassword();
   });

   // Song window events
   Main::Vimpc::EventHandler(Event::SearchResults, [this] (EventData const & Data)
   {
      Ui::SongWindow * const window = CreateSongWindow(Data.name);

      for (auto uri : Data.uris)
      {
         Mpc::Song * song = Main::Library().Song(uri);

         if (song != NULL)
         {
            window->Buffer().Add(song);
         }
      }

      Ui::SongSorter const sorter(settings_.Get(::Setting::Sort));
      window->Buffer().Sort(sorter);
      SetActiveAndVisible(GetWindowFromName(window->Name()));
   });

   Main::Vimpc::EventHandler(Event::PlaylistContents, [this] (EventData const & Data)
   {
      Ui::SongWindow * const window = CreateSongWindow("P:" + Data.name);

      for (auto uri : Data.uris)
      {
         Mpc::Song * song = Main::Library().Song(uri);

         if (song != NULL)
         {
            window->Buffer().Add(song);
         }
      }

      SetActiveAndVisible(GetWindowFromName(window->Name()));
   });

   Main::Vimpc::EventHandler(Event::DatabaseEnabled,  [this] (EventData const & Data)
   {
      if (Data.state)
      {
         mainWindows_[Browse]->Enable();
      } 
      else
      {
         mainWindows_[Browse]->Disable();
         SetVisible(Browse, false);
      }
   });

   // Thread handling of input
   inputThread_ = Thread(QueueInput, commandWindow_);
}

Screen::~Screen()
{
   Running = false;
   inputThread_.join();

   CursesMutex.lock();

   delete pagerWindow_;

   delwin(tabWindow_);
   delwin(progressWindow_);
   delwin(statusWindow_);
   delwin(commandWindow_);

   for (auto window : mainWindows_)
   {
      delete window.second;
   }

   endwin();

   CursesMutex.unlock();
}


int32_t Screen::GetWindowFromName(std::string const & name) const
{
   int32_t index = Unknown;

   for (auto window : mainWindows_)
   {
      if ((window.second != NULL) && (Algorithm::iequals(name, window.second->Name()) == true))
      {
         index = window.first;
      }
   }

   return index;
}


std::string Screen::GetNameFromWindow(int32_t window) const
{
   WindowMap::const_iterator it = mainWindows_.find(window);

   std::string name = "Unknown";

   if (it != mainWindows_.end())
   {
      if (it->second != NULL)
      {
         name = it->second->Name();
      }
   }

   return name;
}


void Screen::Start()
{
   REQUIRE(started_ == false);

   if (started_ == false)
   {
      started_ = true;

      ClearStatus();

      // Mark the default windows as visible
      Regex::RE csv("([^,]+),?");
      std::string window;

      std::string visible = settings_.Get(Setting::Windows);

      std::vector<int32_t>    visibleWindows;
      std::map<int32_t, bool> addedWindows;

      while (csv.Capture(visible, &window))
      {
         visible = visible.substr(window.size());

         int32_t id = GetWindowFromName(window);

         if ((id != static_cast<int32_t>(Unknown)) &&
             (mainWindows_[id] != NULL) &&
             (IsVisible(id) == true) &&
             (addedWindows.find(id) == addedWindows.end()))
         {
            addedWindows[id] = true;
            visibleWindows.push_back(id);
         }
      }

      visibleWindows_ = visibleWindows;

      SetActiveAndVisible(GetWindowFromName(settings_.Get(Setting::Window)));

      if (visibleWindows_.size() == 0)
      {
         SetActiveAndVisible(Playlist);
      }

      CursesMutex.lock();
      wrefresh(statusWindow_);
      CursesMutex.unlock();
   }

   ENSURE(started_ == true);
}


Ui::SongWindow * Screen::CreateSongWindow(std::string const & name)
{
   int32_t id = static_cast<int32_t>(Dynamic);

   while (mainWindows_.find(id) != mainWindows_.end())
   {
      ++id;
   }

   Ui::SongWindow * window = new SongWindow(settings_, *this, client_, clientState_, search_, name);
   mainWindows_[id]        = window;

   return window;
}

Ui::InfoWindow * Screen::CreateInfoWindow(int32_t Id, std::string const & name, Mpc::Song * song)
{
   SetVisible(Id, false);
   Ui::InfoWindow * window = new InfoWindow(song->URI(), settings_, *this, client_, clientState_, search_, name);
   mainWindows_[Id]        = window;
   return window;
}

void Screen::CreateSongInfoWindow(Mpc::Song * song)
{
   if (song != NULL)
   {
      InfoWindow * window = CreateInfoWindow(SongInfo, "info", song);

      if (window->BufferSize() > 0)
      {
         SetActiveAndVisible(GetWindowFromName(window->Name()));
      }
   }
}

#ifdef LYRICS_SUPPORT
Ui::LyricsWindow * Screen::CreateLyricsWindow(int32_t Id, std::string const & name, Mpc::Song * song)
{
   SetVisible(Id, false);
   Ui::LyricsWindow * window = new LyricsWindow(song->URI(), settings_, *this, client_, clientState_, search_, name);
   mainWindows_[Id]          = window;
   return window;
}

void Screen::CreateSongLyricsWindow(Mpc::Song * song)
{
   if (song != NULL)
   {
      LyricsWindow * window = CreateLyricsWindow(Lyrics, "lyrics", song);

      SetActiveAndVisible(GetWindowFromName(window->Name()));
   }
}
#endif

ModeWindow * Screen::CreateModeWindow()
{
   ModeWindow * window = new ModeWindow(maxColumns_, maxRows_);
   modeWindows_.push_back(window);
   return window;
}

void Screen::DeleteModeWindow(ModeWindow * window)
{
   bool found = false;

   for (auto modewindow : modeWindows_)
   {
      if (modewindow == window)
      {
         delete modewindow;
         modeWindows_.remove(modewindow);
         break;
      }
   }
}


void Screen::PromptForPassword()
{
   Error(ErrorNumber::Unknown, "You need a password");
}

PagerWindow * Screen::GetPagerWindow()
{
   return pagerWindow_;
}

void Screen::ShowPagerWindow()
{
   pager_ = true;
   Resize(true);
}

void Screen::PagerWindowNext()
{
   pagerWindow_->Page();
}

void Screen::HidePagerWindow()
{
   pagerWindow_->Clear();
   pager_ = false;
   Resize(true);
}

bool Screen::PagerIsVisible()
{
   return pager_;
}

bool Screen::PagerIsFinished()
{
   return pagerWindow_->IsAtEnd();
}

void Screen::SetStatusLine(char const * const fmt, ...) const
{
   ClearStatus();

   CursesMutex.lock();

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattron(statusWindow_, COLOR_PAIR(settings_.colours.StatusLine));
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

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattroff(statusWindow_, COLOR_PAIR(settings_.colours.StatusLine));
   }
   else
   {
      wattroff(statusWindow_, A_REVERSE);
   }

   wrefresh(statusWindow_);

   CursesMutex.unlock();
}

void Screen::MoveSetStatus(uint16_t x, char const * const fmt, ...) const
{
   CursesMutex.lock();

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattron(statusWindow_, COLOR_PAIR(settings_.colours.StatusLine));
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

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattroff(statusWindow_, COLOR_PAIR(settings_.colours.StatusLine));
   }
   else
   {
      wattroff(statusWindow_, A_REVERSE);
   }

   wrefresh(statusWindow_);

   CursesMutex.unlock();
}

void Screen::SetProgress(double percent)
{
   if ((percent <= 1) && (percent >= 0))
   {
      progress_ = percent;
   }
}


void Screen::Align(Direction direction, uint32_t count)
{
   int64_t  selection  = ActiveWindow().CurrentLine();
   uint32_t min        = ActiveWindow().FirstLine();
   uint32_t max        = MaxRows();
   uint32_t scrollLine = ActiveWindow().CurrentLine();

   if (direction == Up)
   {
      count = (count > min) ? min : count;

      if (selection >= static_cast<int64_t>(min + max - count - 1))
      {
         selection = min + max - count - 1;
      }

      selection = (selection < 0) ? max : selection;
      ActiveWindow().ScrollWindow::Scroll(-1 * count);
   }
   else if (direction == Down)
   {
      if (selection <= static_cast<int64_t>(min + count))
      {
         selection = min + count;
      }

      if (selection > static_cast<int64_t>(ActiveWindow().BufferSize() - max))
      {
         selection = ActiveWindow().BufferSize() - max;
      }

      ActiveWindow().ScrollWindow::Scroll(count);
   }

   // Uses the select window scroll to set the current line properly
   ScrollTo(selection);
}

void Screen::AlignTo(Location location, uint32_t line)
{
   int64_t scrollLine = (line != 0) ? (line - 1) : ActiveWindow().CurrentLine();
   int64_t selection  = scrollLine;

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
   int64_t scrollCount = count;

   if (size == FullPage)
   {
      scrollCount *= (MaxRows());
   }
   else if (size == Page)
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
   uint32_t scroll[LocationCount];

   if (location == Current)
   {
      // This changes the directory windows path
      // so should only be called if we are going to the current song
      ActiveWindow().ScrollToCurrent();
   }

   scroll[Top]          = 0;
   scroll[Bottom]       = ActiveWindow().BufferSize();
   scroll[Current]      = ActiveWindow().Current() + line;
   scroll[PlaylistNext] = ActiveWindow().Playlist(1);
   scroll[PlaylistPrev] = ActiveWindow().Playlist(-1);
   scroll[Specific]     = line - 1;
   srand(time(0));
   scroll[Random]       = (ActiveWindow().BufferSize() > 0) ? (rand() % ActiveWindow().BufferSize()) : 0;

   ScrollTo(scroll[location]);
}

void Screen::ScrollToAZ(std::string const & input)
{
   ActiveWindow().ScrollToFirstMatch(input);
}


void Screen::Clear()
{
   Main::Console().Clear();

   if (window_ == Console)
   {
      werase(mainWindow_);
      Update();
   }
}

void Screen::PrintModeWindow(Ui::ModeWindow * window)
{
   CursesMutex.lock();
   window->Print(0);
   CursesMutex.unlock();
}

void Screen::HideCursor()
{
   CursesMutex.lock();
   curs_set(0);
   CursesMutex.unlock();
}

void Screen::Update()
{
   if ((started_ == true) && (mainWindows_[window_] != NULL))
   {
      WindowMap::iterator it = mainWindows_.begin();

      HideCursor();
      Initialise(window_);

      CursesMutex.lock();
      werase(mainWindow_);
      CursesMutex.unlock();

      // Only paint the tab bar if it is currently visible
      if (settings_.Get(Setting::TabBar) == true)
      {
         UpdateTabWindow();
      }

      if (settings_.Get(Setting::ProgressBar) == true)
      {
         UpdateProgressWindow();
      }

      CursesMutex.lock();

      // Paint the main window
      for (uint32_t i = 0; (i < static_cast<uint32_t>(MaxRows())); ++i)
      {
         ActiveWindow().Print(i);
      }

      wnoutrefresh(mainWindow_);

      // Show the pager window if it is currently enabled
      if (pager_ == true)
      {
         werase(pagerWindow_->N_WINDOW());

         for (uint32_t i = 0; i < pagerWindow_->BufferSize(); ++i)
         {
            pagerWindow_->Print(i);
         }

         wrefresh(pagerWindow_->N_WINDOW());
      }

      doupdate();

      CursesMutex.unlock();
   }
}

void Screen::Redraw()
{
   // Keep track of which windows have been drawn atleast once
   drawn_[window_] = true;
   Redraw(window_);
   Resize(true);
}

void Screen::Redraw(int32_t window) const
{
   WindowMap::const_iterator it = mainWindows_.find(window);

   if ((it != mainWindows_.end()) && (it->second != NULL))
   {
      (it->second)->Redraw();
   }

   // Keep track of which windows have been drawn atleast once
   drawn_[window] = true;
}

void Screen::Initialise(int32_t window) const
{
   if (drawn_[window] == false)
   {
      drawn_[window] = true;
      Redraw(window);
   }
}

void Screen::Invalidate(int32_t window)
{
   drawn_[window] = false;
}

void Screen::InvalidateAll()
{
   for (auto it = mainWindows_.begin(); (it != mainWindows_.end()); ++it)
   {
      if (it->first < static_cast<int>(Dynamic))
      {
         Invalidate(it->first);
      }
      else
      {
         SetVisible(it->first, false, false);
         delete it->second;
         mainWindows_.erase(it++);
      }
   }
}

bool Screen::Resize(bool forceResize, int rows, int cols)
{
   bool WasWindowResized = false;

   if ((WindowResized == true) || (forceResize == true))
   {
      WasWindowResized = true;
      WindowResized    = false;
      CursesMutex.lock();

      // Get the window size with thanks to irssi
#ifdef TIOCGWINSZ
      struct winsize windowSize;

      if ((ioctl(0, TIOCGWINSZ, &windowSize) >= 0) &&
          (windowSize.ws_row >= 0 && windowSize.ws_col >= 0))
      {
         maxRows_    = windowSize.ws_row;
         maxColumns_ = windowSize.ws_col;
      }
#else
      endwin();
      refresh();
      maxRows_    = LINES;
      maxColumns_ = COLS;
#endif

      //! Allows for overriding the resize to a specific dimension
      //! this is useful to test resizing without having to actually
      //! resize the terminal window repeatedly
      if ((rows != -1) && (cols != -1))
      {
         maxRows_    = rows;
         maxColumns_ = cols;
      }

      if (maxRows_ >= 0)
      {
         int32_t topline    = 0;
         int32_t statusline = 0;

         resizeterm(maxRows_, maxColumns_);
         wresize(stdscr, maxRows_, maxColumns_);

         int lastRow = maxRows_ - 1;
         int lines = pagerWindow_->BufferSize();

         if (pager_ == true)
         {
            wresize(pagerWindow_->N_WINDOW(), lines, maxColumns_);
            mvwin(pagerWindow_->N_WINDOW(), maxRows_ - lines, 0);
            wclear(pagerWindow_->N_WINDOW());
            maxRows_ -= (lines - 1);
         }
         else
         {
            wresize(pagerWindow_->N_WINDOW(), 0, maxColumns_);
            wclear(pagerWindow_->N_WINDOW());
         }

         mainRows_ = maxRows_ - 2;

         if (settings_.Get(Setting::TabBar) == true)
         {
            topline = 1;
            mainRows_--;
         }

         if (settings_.Get(Setting::ProgressBar) == true)
         {
            statusline = 1;
            mainRows_--;
         }

         if (mainRows_ < 0)
         {
            mainRows_ = 0;
         }

         wresize(statusWindow_,   1, maxColumns_);
         wresize(tabWindow_,      1, maxColumns_);
         wresize(progressWindow_, 1, maxColumns_);
         mvwin(statusWindow_,   maxRows_ - 2, 0);
         mvwin(progressWindow_, maxRows_ - 2 - statusline, 0);

         for (auto modewindow : modeWindows_)
         {
            modewindow->Resize(1, maxColumns_);
            modewindow->Move(lastRow, 0);
         }

         mvwin(Ui::ErrorWindow::Instance().N_WINDOW(), lastRow, 0);
         mvwin(Ui::ResultWindow::Instance().N_WINDOW(), lastRow, 0);

         if (pager_ == true)
         {
            maxRows_ += (lines - 1);
         }

         wclear(mainWindow_);
         wresize(mainWindow_, mainRows_, maxColumns_);
         mvwin(mainWindow_, topline, 0);

         for (int i = 0; (i < MainWindowCount); ++i)
         {
            if (mainWindows_[i] != NULL)
            {
               uint32_t CurrentLine = mainWindows_[i]->CurrentLine();
               mainWindows_[i]->Resize(mainRows_, maxColumns_);
               mainWindows_[i]->ScrollTo(CurrentLine);
            }
         }

         Update();
         refresh();
      }
      CursesMutex.unlock();
   }

   return WasWindowResized;
}


uint32_t Screen::MaxRows() const
{
   if (mainRows_ >= 0)
   {
      return static_cast<uint32_t>(mainRows_);
   }

   return 0;
}

uint32_t Screen::TotalRows() const
{
   if (maxRows_ >= 0)
   {
      return static_cast<uint32_t>(maxRows_);
   }

   return 0;
}

uint32_t Screen::MaxColumns() const
{
   return maxColumns_;
}

void Screen::UpdateErrorDisplay() const
{
   // \todo this doesn't seem to work if constructed
   // when the screen is constructed, find out why
   Ui::ErrorWindow & errorWindow(Ui::ErrorWindow::Instance());
   Ui::ResultWindow & resultWindow(Ui::ResultWindow::Instance());

   CursesMutex.lock();

   if (errorWindow.HasError() == true)
   {
      errorWindow.Print(0);
   }
   else if (resultWindow.HasResult() == true)
   {
      resultWindow.Print(0);
   }

   CursesMutex.unlock();
}

void Screen::ClearErrorDisplay() const
{
   Ui::ErrorWindow & errorWindow(Ui::ErrorWindow::Instance());
   Ui::ResultWindow & resultWindow(Ui::ResultWindow::Instance());
   errorWindow.ClearError();
   resultWindow.ClearResult();
}

uint32_t Screen::WaitForInput(uint32_t TimeoutMs, bool HandleEscape) const
{
   return ERR;
}


bool Screen::HandleMouseEvent()
{
#ifdef HAVE_MOUSE_SUPPORT
   char     buffer[64];
   wchar_t wbuffer[256];

   if (settings_.Get(Setting::Mouse) == true)
   {
      CursesMutex.lock();

      MEVENT event;

      //! \TODO this seems to scroll quite slowly and not properly at all
      if (getmouse(&event) == OK)
      {
         if ((event.y == 0) && (settings_.Get(Setting::TabBar) == true))
         {
            if (((event.bstate & BUTTON1_CLICKED) == BUTTON1_CLICKED) || ((event.bstate & BUTTON1_DOUBLE_CLICKED) == BUTTON1_DOUBLE_CLICKED))
            {
               int32_t  x = 0;
               uint32_t i = 1;

               for (auto window : visibleWindows_)
               {
                  std::string const name = GetNameFromWindow(static_cast<int32_t>(window));
                  size_t const mblength = mbstowcs(wbuffer, name.c_str(), (name.length() < 256) ? name.length() : 255);
                  std::wstring const wname(wbuffer, mblength);

                  x += wcswidth(wname.c_str(), mblength) + 2;

                  if (settings_.Get(Setting::WindowNumbers) == true)
                  {
                     sprintf(buffer, "[%u]", i);
                     x += strlen(buffer);
                  }

                  if (event.x < x)
                  {
                     SetActiveAndVisible((static_cast<int32_t>(window)));
                     break;
                  }

                  ++i;
               }

               if ((i > visibleWindows_.size()) && ((event.bstate & BUTTON1_DOUBLE_CLICKED) == BUTTON1_DOUBLE_CLICKED))
               {
                  // If a user double clicks an empty spot in the tab bar open the
                  // window selection window
                  SetActiveAndVisible(WindowSelect);
               }
            }
            CursesMutex.unlock();
            return true;
         }
         else if ((event.y >= 0) && (event.y <= static_cast<int32_t>(MaxRows())))
         {
            if (((event.bstate & BUTTON1_CLICKED) == BUTTON1_CLICKED) || ((event.bstate & BUTTON1_DOUBLE_CLICKED) == BUTTON1_DOUBLE_CLICKED))
            {
               int const tabSize   = ((settings_.Get(Setting::TabBar) == true) ? 1 : 0);
               int const titleSize = ((settings_.Get(Setting::ShowPath) == true) && (GetActiveWindow() == Directory)) ? 1 : 0;

               int scroll = ActiveWindow().FirstLine() + event.y - tabSize - titleSize;

               if (scroll < 0)
               {
                  scroll = 0;
               }

               ActiveWindow().ScrollTo(scroll);
            }
         }
         else if ((event.y == static_cast<int32_t>(MaxRows()) + 1) && (settings_.Get(Setting::ProgressBar) == true))
         {
            if (((event.bstate & BUTTON1_CLICKED) == BUTTON1_CLICKED) || ((event.bstate & BUTTON1_DOUBLE_CLICKED) == BUTTON1_DOUBLE_CLICKED))
            {
               OnProgressClicked(event.x);
            }
            CursesMutex.unlock();
            return true;
         }

         event_ = event;
      }

      CursesMutex.unlock();
   }
   return false;
#else
   return false;
#endif
}

void Screen::EnableRandomInput(int count)
{
   RndCount = count;
}


int32_t Screen::GetActiveWindow() const
{
   return window_;
}

int32_t Screen::GetActiveWindowIndex() const
{
   int32_t currentIndex = -1;

   for (uint32_t i = 0; ((i < visibleWindows_.size()) && (currentIndex == -1)); ++i)
   {
      if (visibleWindows_.at(i) == window_)
      {
         currentIndex = i;
      }
   }

   return currentIndex;
}

int32_t Screen::GetPreviousWindow() const
{
   return previous_;
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

int64_t Screen::GetSelected(uint32_t window) const
{
   return Window(window).CurrentLine();
}

Mpc::Song * Screen::GetSong(uint32_t window, uint32_t pos) const
{
   Ui::ScrollWindow & scrollWindow = Window(window);
   Ui::SongWindow * songWindow = dynamic_cast<Ui::SongWindow *>(&scrollWindow);

   if ((songWindow != NULL) && (songWindow->Buffer().Size() > pos))
   {
      return songWindow->Buffer().Get(pos);
   }
   return NULL;
}

void Screen::SetActiveWindowType(MainWindow window)
{
   previous_ = window_;
   window_   = window;
   Update();
}

void Screen::SetActiveWindow(uint32_t window)
{
   if ((window < visibleWindows_.size()))
   {
      previous_ = window_;
      window_   = visibleWindows_.at(window);
   }

   Update();
}

void Screen::SetActiveWindow(Skip skip)
{
   int32_t const currentIndex = GetActiveWindowIndex();
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


bool Screen::IsVisible(int32_t window)
{
   for (auto win : visibleWindows_)
   {
      if (win == window)
      {
         return true;
      }
   }

   return false;
}

void Screen::SetVisible(int32_t window, bool visible, bool removeWindow)
{
   if (mainWindows_[window] != NULL)
   {
      //! \todo Handle the case when there is no visible tabs left and clear the mainwindow
      bool found = false;

      if ((window == window_) && (visible == false))
      {
         bool previous = false;
         int32_t index = -1;

         for (auto it = visibleWindows_.begin(); ((it != visibleWindows_.end()) && (previous == false)); ++it)
         {
            if (((*it) == previous_) && ((*it) != window))
            {
               previous = true;
            }

            index++;
         }

         if (previous == true)
         {
            SetActiveWindow(index);
         }
         else if (visibleWindows_.back() == window)
         {
            SetActiveWindow(Ui::Screen::Previous);
         }
         else
         {
            SetActiveWindow(Ui::Screen::Next);
         }
      }

      for (auto it = visibleWindows_.begin(); ((it != visibleWindows_.end()) && (found == false)); ++it)
      {
         if ((*it) == window)
         {
            found = true;

#ifdef __DEBUG_PRINTS
            if ((RndCount > 0) && (window < MainWindowCount))
            {
               break;
            }
#endif

            if (visible == false)
            {
               visibleWindows_.erase(it);

               if ((window >= Dynamic) && (removeWindow == true))
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

   if (visibleWindows_.size() == 0)
   {
      Main::Vimpc::SetRunning(false);
   }
}

void Screen::SetActiveAndVisible(int32_t window)
{
   if ((mainWindows_[window] != NULL) && (window != static_cast<int32_t>(Unknown)))
   {
      if (mainWindows_[window]->IsEnabled()) 
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
      else
      {
          SetVisible(window, false);
          ErrorString(ErrorNumber::WindowDisabled);
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

      for (auto it = visibleWindows_.begin(); ((it != visibleWindows_.end()) && (found == false)); ++it)
      {
         if ((*it) == window)
         {
            found = true;
            visibleWindows_.erase(it);
         }
      }

      auto it = visibleWindows_.begin();
      for (; ((it != visibleWindows_.end()) && (pos < position)); ++it, ++pos) { }

      if (pos == position)
      {
         visibleWindows_.insert(it, window);
      }
   }
}


void Screen::SetupMouse(bool on) const
{
#ifdef HAVE_MOUSE_SUPPORT
   CursesMutex.lock();

   if (on == true)
   {
      mousemask(ALL_MOUSE_EVENTS, NULL);
   }
   else
   {
      mousemask(0, NULL);
   }

   CursesMutex.unlock();
#endif
}

void Screen::ClearStatus() const
{
   std::string BlankLine(maxColumns_, ' ');

   CursesMutex.lock();

   werase(statusWindow_);

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattron(statusWindow_, COLOR_PAIR(settings_.colours.StatusLine));
   }
   else
   {
      wattron(statusWindow_, A_REVERSE);
   }

   mvwprintw(statusWindow_, 0, 0, BlankLine.c_str());

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattroff(statusWindow_, COLOR_PAIR(settings_.colours.StatusLine));
   }
   else
   {
      wattroff(statusWindow_, A_REVERSE);
   }

   CursesMutex.unlock();
}

void Screen::UpdateTabWindow() const
{
   std::string const BlankLine(maxColumns_, ' ');

   CursesMutex.lock();

   werase(tabWindow_);

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattron(tabWindow_, COLOR_PAIR(settings_.colours.TabWindow));
   }

   mvwprintw(tabWindow_, 0, 0, BlankLine.c_str());
   wmove(tabWindow_, 0, 0);

   std::string name   = "";
   uint32_t    count  = 0;

   for (auto window : visibleWindows_)
   {
      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattron(tabWindow_, COLOR_PAIR(settings_.colours.TabWindow));
      }

      wattron(tabWindow_, A_UNDERLINE);
      name = GetNameFromWindow(static_cast<int32_t>(window));

      if (window == window_)
      {
         if (settings_.Get(Setting::ColourEnabled) == true)
         {
            wattron(tabWindow_, COLOR_PAIR(settings_.colours.TabWindow));
         }

         wattron(tabWindow_, A_REVERSE | A_BOLD);
      }

      if (settings_.Get(Setting::WindowNumbers) == true)
      {
         waddstr(tabWindow_, "[");
         wattron(tabWindow_, A_BOLD);
         wprintw(tabWindow_, "%d", count + 1);

         if (window != window_)
         {
            wattroff(tabWindow_, A_BOLD);
         }

         waddstr(tabWindow_, "]");
      }

      wprintw(tabWindow_, " %s ", name.c_str());
      wattroff(tabWindow_, A_REVERSE | A_BOLD);
      ++count;
   }

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattroff(tabWindow_, COLOR_PAIR(settings_.colours.TabWindow));
   }

   wattroff(tabWindow_, A_UNDERLINE);
   wrefresh(tabWindow_);

   CursesMutex.unlock();
}

void Screen::UpdateProgressWindow() const
{
   CursesMutex.lock();

   werase(progressWindow_);

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattron(progressWindow_, COLOR_PAIR(settings_.colours.ProgressWindow));
   }

   wmove(progressWindow_, 0, 0);
   whline(progressWindow_, 0, MaxColumns());
   wmove(progressWindow_, 0, 0);

   wattron(progressWindow_, A_BOLD);

   if ((progress_ * MaxColumns() - 1) > 0)
   {
      whline(progressWindow_, '=', static_cast<int>(progress_ * MaxColumns() - 1));
      wmove(progressWindow_, 0, static_cast<int>(progress_ * MaxColumns() - 1));
   }
   if (progress_ > 0)
   {
      waddch(progressWindow_, '>');
   }


   if (settings_.Get(Setting::ShowPercent) == true)
   {
      uint32_t start = (MaxColumns() / 2) - 3;

      if (start + 6 < MaxColumns())
      {
         wmove(progressWindow_, 0, start);
         wprintw(progressWindow_, "[%3d%%]", static_cast<int>(progress_ * 100));
      }
   }

   wattroff(progressWindow_, A_BOLD);

   if (settings_.Get(Setting::ColourEnabled) == true)
   {
      wattroff(progressWindow_, COLOR_PAIR(settings_.colours.ProgressWindow));
   }

   wattroff(progressWindow_, A_UNDERLINE);
   wrefresh(progressWindow_);

   CursesMutex.unlock();
}


void Screen::RegisterProgressCallback(ProgressCallback callback)
{
   pCallbacks_.push_back(callback);
}

void Screen::OnProgressClicked(int32_t x)
{
   if (settings_.Get(Setting::SeekBar) == true)
   {
      // Call any registered callbacks for a progress click
      FOREACH (auto func, pCallbacks_)
      {
         (func)((static_cast<double>(x) / MaxColumns()));
      }
   }
}

void Screen::OnTabSettingChange(bool Value)
{
   // Force a resize of the windows if the tab bar was recently
   // hidden or shown
   Resize(true);
}

void Screen::OnProgressSettingChange(bool Value)
{
   // Force a resize of the windows if the progress bar was recently
   // hidden or shown
   Resize(true);
}

void Screen::OnMouseSettingChange(bool Value)
{
   // If the mouse setting was changed reinitialise
   SetupMouse(Value);
}

void ResizeHandler(int i)
{
   WindowResized = true;
}

void ContinueHandler(int i)
{
   CursesMutex.lock();
   refresh();
   CursesMutex.unlock();
   Continue = true;
   WindowResized = true;
}
/* vim: set sw=3 ts=3: */
