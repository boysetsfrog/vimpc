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

#include "dbc.hpp"
#include "modewindow.hpp"

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
   class PlaylistWindow;

   class Screen
   {
   public:
      Screen(Mpc::Client & client, Main::Settings const & settings);
      ~Screen();

   public:
      typedef enum
      {
         Console,
         Help,
         Playlist,
         Library,
         MainWindowCount
      } MainWindow;
   
      typedef enum 
      { 
         Current, 
         Top, 
         Bottom,
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

   public:
      void Start();
      ModeWindow * CreateModeWindow();
      void SetStatusLine(char const * const fmt, ... );

   public:
      void Scroll(Size size, Direction direction, uint32_t count);
      void ScrollTo(Location location);
      void Search(std::string const & searchString) const;

   public:
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
      void SetDefaultWindow(MainWindow window);
      Ui::ConsoleWindow  & ConsoleWindow()  const;
      Ui::PlaylistWindow & PlaylistWindow() const; 

   public:
      static MainWindow GetWindowFromName(std::string const & windowName);

   private:
      void Scroll(int32_t count);
      void ScrollTo(uint32_t line);
      bool WindowsAreInitialised();

   private:
      typedef std::map<std::string, Ui::Screen::MainWindow> WindowTable;

   private:
      MainWindow           window_;
      MainWindow           defaultWindow_;
      Ui::PlaylistWindow * playlistWindow_;
      Ui::ConsoleWindow  * consoleWindow_;
      Window             * libraryWindow_;
      Window             * helpWindow_;
      Window             * mainWindows_[MainWindowCount];
      WINDOW             * statusWindow_;
      WINDOW             * commandWindow_;

      Mpc::Client    const & client_;
      Main::Settings const & settings_;

      bool        started_;
      uint32_t    maxRows_;
      uint32_t    maxColumns_;
   };
}
#endif
