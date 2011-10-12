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

//#define HAVE_MOUSE_SUPPORT

// Forward declarations
namespace Main
{
   class Settings;
}

namespace Mpc
{
   class Client;
   class Song;
}

namespace Ui
{
   class ConsoleWindow;
   class InfoWindow;
   class LibraryWindow;
   class PlaylistWindow;
   class Search;
   class SongWindow;
}

// Screen management class
namespace Ui
{
   class Screen
   {
   public:
      Screen(Main::Settings & settings, Mpc::Client & client, Search const & search);
      ~Screen();

   private:
      Screen(Screen & screen);
      Screen & operator=(Screen & screen);

   public:
      typedef std::map<int32_t, ScrollWindow *> WindowMap;

      // Tabs/Windows that can be used
      typedef enum
      {
         Help = 0,
         Console,
         Outputs,
         Library,
         Lists,
         Browse,
         Playlist,
         MainWindowCount,

         Unknown,
         Dynamic //Anything above dynamic is a dynamic window
      } MainWindow;

      // Scroll/Selection locations within a window
      typedef enum
      {
         Current,
         Top,
         Bottom,
         Centre,
         Specific,
         PlaylistNext,
         PlaylistPrev,
         LocationCount
      } Location;

      // Scroll sizes
      typedef enum { Line, Page } Size;

      // Scroll directions
      typedef enum { Up, Down } Direction;

      // Navigation directions
      typedef enum { Next, Previous, Absolute } Skip;

   public:
      // Get the window value given a window name
      int32_t GetWindowFromName(std::string const & windowName) const;

      // Get the window name given the value
      std::string GetNameFromWindow(int32_t window) const;

   public:
      // Set the correct window to be active, flag screen as started
      void Start();

      // Create a new song window, usually used for search results
      Ui::SongWindow * CreateSongWindow(std::string const & name);
      Ui::InfoWindow * CreateInfoWindow(std::string const & name, Mpc::Song * song = NULL);

      // Create a new window used to display information specific to the currently active mode
      ModeWindow * CreateModeWindow();
      void DeleteModeWindow(ModeWindow * window);

      // Update the status line to indicate currently playing song, etc
      void SetStatusLine(char const * const fmt, ... ) const;
      void MoveSetStatus(uint16_t x, char const * const fmt, ... ) const;

   public:
      // Align the current window up or down( ^E, ^Y )
      void Align(Direction direction, uint32_t count = 0);

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
      void Redraw(int32_t window) const;

      // Handle a screen resize
      bool Resize(bool forceResize = false);

   public:
      uint32_t MaxRows()      const;
      uint32_t MaxColumns()   const;
      uint32_t WaitForInput() const;
      void     ClearInput()   const;

      void HandleMouseEvent();

   public:
      // Access the active window
      int32_t GetActiveWindow() const;
      Ui::ScrollWindow & ActiveWindow() const;
      Ui::ScrollWindow & Window(uint32_t window) const;

      // Changes the currently active window by setting it explicitly
      void SetActiveWindow(uint32_t window);

      // Changes the currently active window by rotating through those available
      void SetActiveWindow(Skip skip);

      // Show or hide the given window
      void SetVisible(int32_t window, bool visible);
      uint32_t VisibleWindows() { return visibleWindows_.size(); }

      // Show a given window and make it active
      void SetActiveAndVisible(int32_t window);

      // Move the window to a new position
      void MoveWindow(uint32_t position);
      void MoveWindow(int32_t window, uint32_t position);

   private:
      void ClearStatus() const;
      void UpdateTabWindow() const;

   private:
      int32_t    window_;
      WindowMap  mainWindows_;
      WINDOW *   statusWindow_;
      WINDOW *   tabWindow_;
      WINDOW *   commandWindow_;

      std::vector<int32_t>      visibleWindows_;
      std::vector<ModeWindow *> modeWindows_;

      bool      started_;
      uint32_t  maxRows_;
      uint32_t  maxColumns_;

      Main::Settings &   settings_;
      Mpc::Client &      client_;
      Ui::Search const & search_;
   };
}
#endif
