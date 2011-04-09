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

   screen.hpp - ncurses window management
   */

#ifndef __UI__SCREEN
#define __UI__SCREEN

// Includes
#include "assert.hpp"
#include "buffer/linebuffer.hpp"
#include "window/modewindow.hpp"
#include "window/scrollwindow.hpp"

#include <string>

// Forward declarations
namespace Main { class Settings; }
namespace Mpc  { class Client; }
namespace Ui   { class Search; class ConsoleWindow; class LibraryWindow; class PlaylistWindow; }

// Screen management class
namespace Ui
{
   class Screen
   {
   public:
      Screen(Main::Settings const & settings, Mpc::Client & client, Search const & search);
      ~Screen();

   private:
      Screen(Screen & screen);
      Screen & operator=(Screen & screen);

   public:
      // Tabs/Windows that can be used
      typedef enum { Help = 0, Console, Library, Browse, Playlist, MainWindowCount } MainWindow;
   
      // Scroll/Selection locations within a window
      typedef enum { Current, Top, Bottom, Centre, Specific, LocationCount } Location;
      
      // Scroll sizes
      typedef enum { Line, Page } Size;
      
      // Scroll directions
      typedef enum { Up, Down } Direction;

      // Navigation directions
      typedef enum { Next, Previous } Skip;

   public:
      // Get the window value given a window name
      static MainWindow  GetWindowFromName(std::string const & windowName);

      // Get the window name given the value
      static std::string GetNameFromWindow(MainWindow window);

   public:
      // Set the correct window to be active, flag screen as started 
      void Start();

      // Create a new window used to display information specific to the currently active mode
      ModeWindow * CreateModeWindow();
      void DeleteModeWindow(ModeWindow * window);

      // Update the status line to indicate currently playing song, etc
      void SetStatusLine(char const * const fmt, ... ) const;
      void MoveSetStatus(uint16_t x, char const * const fmt, ... ) const;

   public:
      // Align the currently selected line to a given location on the screen (z<CR>, z-, z.)
      void AlignTo(Location location, uint32_t line = 0);

      // Select a given (currently visible) line (H, L, M)
      void Select(ScrollWindow::Position position, uint32_t count);

      // Scroll the window to a location or by a given amount
      void Scroll(int32_t count);
      void Scroll(Size size, Direction direction, uint32_t count);
      void ScrollTo(uint32_t line);
      void ScrollTo(Location location, uint32_t line = 0);

   public:
      // Clear the console window
      void Clear();

      // Reprint the currently active main window
      void Update();

      // Reinitialise the given main window, ie rebuild playlist, library, etc
      void Redraw() const;
      void Redraw(MainWindow window) const;

      // Handle a screen resize
      bool Resize(bool forceResize = false);

   public:
      uint32_t MaxRows()      const;
      uint32_t MaxColumns()   const;
      uint32_t WaitForInput() const;

   public:
      // Access the active window
      MainWindow GetActiveWindow() const;
      Ui::ScrollWindow   & ActiveWindow() const;

      // Changes the currently active window by setting it explicitly
      void SetActiveWindow(uint32_t window);

      // Changes the currently active window by rotating through those available
      void SetActiveWindow(Skip skip);

      // Show or hide the given window
      void SetVisible(MainWindow window, bool visible);
      uint32_t VisibleWindows() { return visibleWindows_.size(); }

      // Show a given window and make it active
      void SetActiveAndVisible(MainWindow window);

      // Move the window to a new position
      void MoveWindow(uint32_t position);
      void MoveWindow(MainWindow window, uint32_t position);
   
   private:
      void ClearStatus() const;
      void UpdateTabWindow() const;
      bool WindowsAreInitialised();

   private:
      MainWindow             window_;
      ScrollWindow         * mainWindows_[MainWindowCount];
      WINDOW               * statusWindow_;
      WINDOW               * tabWindow_;
      WINDOW               * commandWindow_;

      std::vector<MainWindow>   visibleWindows_;
      std::vector<ModeWindow *> modeWindows_;

      bool                   started_;
      uint32_t               maxRows_;
      uint32_t               maxColumns_;

      Main::Settings const & settings_;
   };
}
#endif
