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

   vimpc.hpp - handles mode changes and input processing
   */

#ifndef __MAIN__VIMPC
#define __MAIN__VIMPC

#include <map>

#include "compiler.hpp"

#include "clientstate.hpp"
#include "mpdclient.hpp"
#include "screen.hpp"

struct EventData;
namespace Ui   { class Mode; class Normal; class Command; }
namespace Main { class Settings; }

namespace Main
{
   class Vimpc
   {
		static std::string LyricsArtist;
		static std::string LyricsTitle;

   public:
      Vimpc();
      ~Vimpc();

   public:
      //! All available modes
      typedef enum
      {
         Command,
         Normal,
         Search,
         ModeCount
      } ModeName;

   public:
      //! Start vimpc
      void Run(std::string hostname = "", uint16_t port = 0);

      //! Return the currently active mode
      Ui::Mode & CurrentMode();

      //! Check if the given \p input will require the curently active mode
      //! to be changed
      bool RequiresModeChange(ModeName mode, int input) const;

      //! Determines the mode that will be active after the next call to
      //! ChangeMode with the \p input given
      ModeName ModeAfterInput(ModeName mode, int input) const;

      void ChangeMode(char input, std::string initial);

      //! Change whether user events are handled
      void HandleUserEvents(bool Enabled);

   public:
      static void SetRunning(bool isRunning);
      static void CreateEvent(int Event, EventData const & Data);
      static void EventHandler(int Event, FUNCTION<void(EventData const &)> func);
      static bool WaitForEvent(int Event, int TimeoutMs);

   private:
      //! Read input from the screen
      int  Input() const;

      //! Handle the input using the currently active mode
      void Handle(int input);

      //! Handle a mouse event
      bool HandleMouse();

      //! Checks that each mode is valid
      //! \todo Possibly template this kind of check?
      bool ModesAreInitialised();

      // Flag that a repaint is required
      void SetRepaint(bool requireRepaint);

      // Update the screen
      void Repaint();

   private:
      //! Change the currently active mode based on \p input
      void ChangeMode(int input);

      //! Hook vimpc's command parser
      void SetSkipConfigConnects(bool val);

   private:
      typedef std::map<ModeName, Ui::Mode *> ModeTable;

   private:
      static bool Running;

   private:
      ModeName          currentMode_;
      Settings     &    settings_;
      Ui::Search   &    search_;
      Ui::Screen        screen_;
      Mpc::Client       client_;
      Mpc::ClientState  clientState_;
      ModeTable         modeTable_;
      Ui::Normal   &    normalMode_;
      Ui::Command  &    commandMode_;
      Atomic(bool)      userEvents_;
      bool              requireRepaint_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
