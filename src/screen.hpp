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

#include "assert.hpp"
#include "modewindow.hpp"
#include "scrollwindow.hpp"

#include <map>
#include <string>

namespace Main
{
   class Settings;
}

namespace Mpc
{
   class Client;
}

namespace Ui
{
   class Window;
   class ConsoleWindow;
   class ErrorWindow;
   class LibraryWindow;
   class PlaylistWindow;
   class Search;

   class Screen
   {
   public:
      Screen(Main::Settings const & settings, Mpc::Client & client, Ui::Search const & search);
      ~Screen();

   private:
      Screen(Screen & screen);
      Screen & operator=(Screen & screen);

   public:
      typedef enum
      {
         Help = 0,
         Console,
         Playlist,
         Library,
         MainWindowCount
      } MainWindow;
   
      typedef enum 
      { 
         Current, 
         Top, 
         Bottom,
         Specific,
         LocationCount
      } Location;
      
      typedef enum 
      { 
         Line, 
         Page 
      } Size;
      
      typedef enum 
      { 
         Up, 
         Down 
      } Direction;

      typedef enum 
      { 
         Next, 
         Previous 
      } Skip;

   public:
      void Start();
      ModeWindow * CreateModeWindow();

      void SetTopWindow() const;
      void ClearStatus() const;
      void SetStatusLine(char const * const fmt, ... ) const;
      void MoveSetStatus(uint16_t x, char const * const fmt, ... ) const;

   public:
      void Select(ScrollWindow::Position position, uint32_t count);
      void Scroll(Size size, Direction direction, uint32_t count);
      void ScrollTo(Location location, uint32_t line = 0);

   public:
      void Left(Ui::Player & player, uint32_t count);
      void Right(Ui::Player & player, uint32_t count);
      void Confirm();
      void Clear();
      void Update() const;
      void Redraw();

   public:
      uint32_t MaxRows()      const;
      uint32_t MaxColumns()   const;
      uint32_t WaitForInput() const;

   public:
      void SetActiveWindow(MainWindow window);
      void SetActiveWindow(Skip skip);
      void SetDefaultWindow(MainWindow window);
      Ui::ScrollWindow   & ActiveWindow() const;
      Ui::ConsoleWindow  & ConsoleWindow() const;
      Ui::LibraryWindow  & LibraryWindow() const; 
      Ui::PlaylistWindow & PlaylistWindow() const; 

   public:
      static MainWindow GetWindowFromName(std::string const & windowName);
      static std::string GetNameFromWindow(MainWindow window);

      void Scroll(int32_t count);
      void ScrollTo(uint32_t line);

   private:
      bool WindowsAreInitialised();

   private:
      typedef std::map<std::string, Ui::Screen::MainWindow> WindowTable;

   private:
      MainWindow           window_;
      Ui::PlaylistWindow * playlistWindow_;
      Ui::ConsoleWindow  * consoleWindow_;
      Ui::LibraryWindow  * libraryWindow_;
      ScrollWindow       * helpWindow_;
      ScrollWindow       * mainWindows_[MainWindowCount];
      WINDOW             * statusWindow_;
      WINDOW             * topWindow_;
      WINDOW             * commandWindow_;

      Main::Settings const & settings_;

      bool        started_;
      uint32_t    maxRows_;
      uint32_t    maxColumns_;
   };
}
#endif
