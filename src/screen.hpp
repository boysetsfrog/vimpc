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
#include "config.h"
#include "buffers.hpp"
#include "buffer/buffer.hpp"
#include "buffer/linebuffer.hpp"
#include "window/modewindow.hpp"
#include "window/pagerwindow.hpp"
#include "window/scrollwindow.hpp"

#include <list>
#include <string>

// Changed to being on by default
// will need to check ncurses properly
#define HAVE_MOUSE_SUPPORT

// Forward declarations
namespace Main
{
   class Settings;
}

namespace Mpc
{
   class Client;
   class ClientState;
   class Song;
}

namespace Ui
{
   class ConsoleWindow;
   class InfoWindow;
   class LibraryWindow;
#ifdef LYRICS_SUPPORT
   class LyricsWindow;
#endif
   class PlaylistWindow;
   class Search;
   class SongWindow;
   class Screen;
}

// Screen management class
namespace Ui
{
   class Windows : public Main::Buffer<uint32_t>
   {
      public:
         Windows(Ui::Screen * screen) :
            screen_(screen) { }

      public:
         std::string String(uint32_t position) const;
         std::string PrintString(uint32_t position) const;

      private:
         Ui::Screen * const screen_;
   };

   class Screen
   {
   public:
      typedef FUNCTION<void (double)> ProgressCallback;

   public:
      Screen(Main::Settings & settings, Mpc::Client & client, Mpc::ClientState & clientState, Search const & search);
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
         DebugConsole,
         TestConsole,
         Console,
         Outputs,
         Library,
         Lists,
         Browse,
         Directory,
         Playlist,
         WindowSelect,
#ifdef LYRICS_SUPPORT
         Lyrics,
#endif
         SongInfo,
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
         Random,
         LocationCount
      } Location;

      // Scroll sizes
      typedef enum { Line, Page, FullPage } Size;

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
      Ui::InfoWindow * CreateInfoWindow(int32_t Id, std::string const & name, Mpc::Song * song = NULL);
      void CreateSongInfoWindow(Mpc::Song * song = NULL);

#ifdef LYRICS_SUPPORT
      Ui::LyricsWindow * CreateLyricsWindow(int32_t Id, std::string const & name, Mpc::Song * song = NULL);
      void CreateSongLyricsWindow(Mpc::Song * song = NULL);
#endif

      // Create a new window used to display information specific to the currently active mode
      ModeWindow * CreateModeWindow();
      void DeleteModeWindow(ModeWindow * window);

      // Prompt for a password from the user
      void PromptForPassword();

      // Pager window used to display maps, settings, etc
      PagerWindow * GetPagerWindow();
      void ShowPagerWindow();
      void HidePagerWindow();
      void PagerWindowNext();
      bool PagerIsVisible();
      bool PagerIsFinished();

      // Update the status line to indicate currently playing song, etc
      void SetStatusLine(char const * const fmt, ... ) const;
      void MoveSetStatus(uint16_t x, char const * const fmt, ... ) const;

      // Set the window's progress bar location
      void SetProgress(double percent);

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
      void ScrollToAZ(std::string const & input);

   public:
      // Clear the console window
      void Clear();

      // Print the mode window
      void PrintModeWindow(Ui::ModeWindow * window);

      // Hide the mode cursor
      void HideCursor();

      // Reprint the currently active main window
      void Update();

      // Reinitialise the given main window, ie rebuild playlist, library, etc
      void Redraw();
      void Redraw(int32_t window) const;
      void Initialise(int32_t window) const;
      void Invalidate(int32_t window);
      void InvalidateAll();

      // Handle a screen resize
      bool Resize(bool forceResize = false, int rows = -1, int cols = -1);

   public:
      //! \TODO this functions need a refactor/rename
      uint32_t MaxRows()      const;
      uint32_t MaxColumns()   const;
      uint32_t TotalRows()    const;
      uint32_t WaitForInput(uint32_t TimeoutMs, bool HandleEscape = true) const;

		void UpdateErrorDisplay() const;
		void ClearErrorDisplay() const;
      bool HandleMouseEvent();

      void EnableRandomInput(int count);

#ifdef HAVE_MOUSE_SUPPORT
      MEVENT LastMouseEvent() { return event_; }
#endif

      WINDOW * W_MainWindow() { return mainWindow_; }

   public:
      // Access the active window
      int32_t GetActiveWindow() const;
      int32_t GetActiveWindowIndex() const;

      Ui::ScrollWindow & ActiveWindow() const;
      Ui::ScrollWindow & Window(uint32_t window) const;

      // Access the previous window
      int32_t GetPreviousWindow() const;

      // Access the selected item in a particular window
      int64_t GetSelected(uint32_t window) const;
      int64_t GetActiveSelected() const { return GetSelected(GetActiveWindow()); }

      // Access a song based on a particular window
      Mpc::Song * GetSong(uint32_t window, uint32_t pos) const;
      Mpc::Song * GetSong(uint32_t pos) const { return GetSong(GetActiveWindow(), pos); }

      // Changes the currently active window by setting it explicitly
      void SetActiveWindowType(MainWindow window);
      void SetActiveWindow(uint32_t window);

      // Changes the currently active window by rotating through those available
      void SetActiveWindow(Skip skip);

      // Show or hide the given window
      bool IsVisible(int32_t window);
      void SetVisible(int32_t window, bool visible, bool removeWindow = true);

      uint32_t VisibleWindows() { return visibleWindows_.size(); }

      // Show a given window and make it active
      void SetActiveAndVisible(int32_t window);

      // Move the window to a new position
      void MoveWindow(uint32_t position);
      void MoveWindow(int32_t window, uint32_t position);

      // Register a callback to occur when progress bar is clicked
      void RegisterProgressCallback(ProgressCallback callback);

      void UpdateProgressWindow() const;

   private:
      void SetupMouse(bool on) const;
      void ClearStatus() const;
      void UpdateTabWindow() const;

   private:
      void OnProgressClicked(int32_t);

      // Settings callbacks
      void OnTabSettingChange(bool);
      void OnProgressSettingChange(bool);
      void OnMouseSettingChange(bool);

   private:
      int32_t    window_;
      int32_t    previous_;
      WindowMap  mainWindows_;
      WINDOW *   mainWindow_;
      WINDOW *   statusWindow_;
      WINDOW *   tabWindow_;
      WINDOW *   progressWindow_;
      WINDOW *   commandWindow_;
      PagerWindow * pagerWindow_;

		Thread	  inputThread_;

      std::vector<int32_t>      visibleWindows_;
      std::list<ModeWindow *>   modeWindows_;
      mutable std::map<int32_t, bool> drawn_;

      std::vector<ProgressCallback> pCallbacks_;

      bool      started_;
      bool      pager_;
      double    progress_;
      int32_t   maxRows_;
      int32_t   mainRows_;
      int32_t   maxColumns_;

#ifdef HAVE_MOUSE_SUPPORT
      MEVENT    event_;
#endif

      Ui::Windows        windows_;
      Main::Settings &   settings_;
      Mpc::Client &      client_;
      Mpc::ClientState & clientState_;
      Ui::Search const & search_;
   };
}
#endif
/* vim: set sw=3 ts=3: */
