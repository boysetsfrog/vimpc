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

   normal.hpp - normal mode input handling 
   */

#ifndef __UI__NORMAL
#define __UI__NORMAL

#include <map>

//! \todo try and reduce the number of headers required
//!       this is due to the amount of templated functions in this file
#include "attributes.hpp"
#include "library.hpp"
#include "mode.hpp"
#include "modewindow.hpp"
#include "player.hpp"
#include "playlist.hpp"
#include "screen.hpp"
#include "scrollwindow.hpp"
#include "search.hpp"
#include "song.hpp"

namespace Main
{
   class Vimpc;
   class Settings;
}

namespace Mpd
{
   class Client;
}

namespace Ui
{
   // Handles all input received whilst in normal mode
   class Normal : public Mode, public Player
   {
   public:
      Normal(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings, Ui::Search & search);
      ~Normal();

   private:
      Normal(Normal & normal);
      Normal & operator=(Normal & normal);

   public: // Ui::Mode
      void Initialise(int input);
      void Finalise(int input);
      void Refresh();
      bool Handle(int input);
      bool CausesModeToStart(int input) const;

   private: // Ui::Player wrapper functions
      bool ClearScreen(uint32_t count);
      bool Pause(uint32_t count);
      bool Random(uint32_t count);
      bool Stop(uint32_t count);

   private:
      bool Left(uint32_t count);
      bool Right(uint32_t count);
      bool Confirm(uint32_t count);
      bool RepeatLastAction(uint32_t count);

   private:
      template <Mpc::Song::SongCollection COLLECTION>
      bool AddSong(uint32_t count);

      template <Mpc::Song::SongCollection COLLECTION>
      bool DeleteSong(uint32_t count);

   private: //Selecting
      template <ScrollWindow::Position POSITION>
      bool Select(uint32_t count); 

   private: //Searching
      template <Search::Skip SKIP>
      bool SearchResult(uint32_t count); 

   private: //Skipping
      template <Player::Skip SKIP>
      bool SkipSong(uint32_t count); 

      template <Player::Skip SKIP>
      bool SkipAlbum(uint32_t count); 

      template <Player::Skip SKIP>
      bool SkipArtist(uint32_t count); 

   private: //Scrolling
      template <Screen::Size SIZE, Screen::Direction DIRECTION>
      bool Scroll(uint32_t count);

      template <Screen::Location LOCATION>
      bool ScrollTo(uint32_t line);

      template <Screen::Location SPECIFIC, Screen::Location ENDLOCATION>
      bool ScrollTo(uint32_t line);

   private: //Windows
      template <Screen::Skip SKIP>
      bool SetActiveWindow(uint32_t count);

   private:
      void DisplayModeLine();

   private:
      ModeWindow *  window_;
      uint32_t      actionCount_;
      int32_t       lastAction_;
      uint32_t      lastActionCount_;
      bool          wasSpecificCount_;

      typedef bool (Ui::Normal::*ptrToMember)(uint32_t);
      typedef std::map<int, ptrToMember> ActionTable;
      ActionTable   actionTable_;
      ActionTable   jumpTable_;

      Ui::Search        & search_;
      Ui::Screen        & screen_;
      Mpc::Client       & client_;
      Main::Settings    & settings_;

   };

   //Implemenation of library functions
   template <Mpc::Song::SongCollection COLLECTION>
   bool Normal::AddSong(uint32_t count)
   {
      //! \todo use count to add more than one song
      //! \todo handle adding all songs
      (void) count;

      screen_.LibraryWindow().AddSongsToPlaylist(COLLECTION);
      return true;
   }

   template <Mpc::Song::SongCollection COLLECTION>
   bool Normal::DeleteSong(uint32_t count)
   {
      //! \todo Make delete and add take a movement operation?
      //!       ie to do stuff like dG, this may require making some kind of movement
      //!          table or something rather than the way it currently works
      //! \todo handle deleting all songs
      screen_.PlaylistWindow().RemoveSong(count);
      Player::Redraw();
      return true;
   }


   // \todo this should be implemented using the window
   // somehow
   //Implementation of selecting functions
   template <ScrollWindow::Position POSITION>
   bool Normal::Select(uint32_t count)
   {
      screen_.Select(POSITION, count);
      return true;
   }


   //Implementation of skipping functions
   template <Ui::Player::Skip SKIP>
   bool Normal::SkipSong(uint32_t count)
   {
      return Player::SkipSong(SKIP, count);
   }

   template <Ui::Player::Skip SKIP>
   bool Normal::SkipAlbum(uint32_t count)
   {
      return Player::SkipAlbum(SKIP, count);
   }

   template <Ui::Player::Skip SKIP>
   bool Normal::SkipArtist(uint32_t count)
   {
      return Player::SkipArtist(SKIP, count);
   }


   //Implementation of searching functions
   template <Ui::Search::Skip SKIP>
   bool Normal::SearchResult(uint32_t count)
   {
      return search_.SearchResult(SKIP, count);
   }


   // Implementation of scrolling functions
   template <Screen::Size SIZE, Screen::Direction DIRECTION>
   bool Normal::Scroll(uint32_t count)
   {
      screen_.Scroll(SIZE, DIRECTION, count);
      return true;
   }

   template <Screen::Location LOCATION>
   bool Normal::ScrollTo(UNUSED uint32_t line)
   {
      screen_.ScrollTo(LOCATION);
      return true;
   }

   template <Screen::Location SPECIFIC, Screen::Location ENDLOCATION>
   bool Normal::ScrollTo(uint32_t line)
   {
      if ((SPECIFIC == Screen::Specific) && (wasSpecificCount_ == false))
      {
         ScrollTo<ENDLOCATION>(line);
      }
      else
      {
         screen_.ScrollTo(SPECIFIC, line);
      }

      return true;
   }



   // Implementation of window functions
   template <Screen::Skip SKIP>
   bool Normal::SetActiveWindow(uint32_t count)
   {
      if ((SKIP == Screen::Next) && (wasSpecificCount_ == true))
      {
         screen_.SetActiveWindow(static_cast<Screen::MainWindow>(count - 1));
      }
      else if ((SKIP == Screen::Previous) && (wasSpecificCount_ == true))
      {
         count = (count % Screen::MainWindowCount);

         for (uint32_t i = 0; i < count; ++i)
         {
            screen_.SetActiveWindow(SKIP);
         }
      }
      else
      {
         screen_.SetActiveWindow(SKIP);
      }

      return true;
   }

}

#endif
