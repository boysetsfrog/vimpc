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

#include "callback.hpp"
#include "screen.hpp"
#include "search.hpp"
#include "vimpc.hpp"

namespace Main
{
   class Settings;
}

namespace Mpc
{
   class Playlist;
}

namespace Ui
{
   // Handles all input received whilst in normal mode
   class Normal : public Mode, public Player
   {
   public:
      Normal(Main::Vimpc * vimpc, Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings, Ui::Search & search);
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
      bool CausesModeToEnd(int input) const;
      bool WaitingForMoreInput() const { return (input_.size() > 0); }

   public:
      // Map a key combination to any other key combination
      void Map(std::string key, std::string mapping);

      // Remove a key mapping
      void Unmap(std::string key);

      // Get the current mappings
      typedef std::map<std::string, std::string> MapNameTable;
      MapNameTable Mappings();

   private:
      // Check the action or map table for a particular a key combination
      template<typename T>
      bool CheckTableForInput(T table, std::string const & toMap, std::string & result);

      // Handle the execution of a complete input command
      bool Handle(std::string input, int count);

      // Handle the execution of a key combination that corresponds to an
      // entry in the key map
      void HandleMap(std::string input, int count);

      // Convert an input character into it's escaped string name
      std::string InputCharToString(int input) const;

   private: // Ui::Player wrapper functions
      void ClearScreen(uint32_t count);
      void Pause(uint32_t count);
      void Stop(uint32_t count);

      void Consume(uint32_t count);
      void Crossfade(uint32_t count);
      void Random(uint32_t count);
      void Repeat(uint32_t count);
      void Single(uint32_t count);

      // Change the volume by count lots of Delta
      template <int Delta>
      void ChangeVolume(uint32_t count);

   private:
      template <Ui::Player::Location LOCATION>
      void SeekTo(uint32_t count);
      void Left(uint32_t count);
      void Right(uint32_t count);
      void Confirm(uint32_t count);
      void Escape(uint32_t count);

      // Execute the last action again
      void RepeatLastAction(uint32_t count);

   private:
      void Expand(uint32_t count);
      void Collapse(uint32_t count);

   private:
      void Close(uint32_t count);
      void Edit(uint32_t count);
      void Visual(uint32_t count);

   private:
      template <Mpc::Song::SongCollection COLLECTION>
      void AddSong(uint32_t count);

      template <Mpc::Song::SongCollection COLLECTION>
      void DeleteSong(uint32_t count);

      template <Mpc::Song::SongCollection COLLECTION>
      void CropSong(uint32_t count);

      void PasteBuffer(uint32_t count);

   private: //Selecting
      template <ScrollWindow::Position POSITION>
      void Select(uint32_t count);

   private: //Searching
      template <Search::Skip SKIP>
      void SearchResult(uint32_t count);

   private: //Skipping
      template <Player::Skip SKIP>
      void SkipSong(uint32_t count);

      template <Player::Skip SKIP>
      void SkipAlbum(uint32_t count);

      template <Player::Skip SKIP>
      void SkipArtist(uint32_t count);

   private: //Scrolling
      template <int8_t OFFSET>
      void ScrollToCurrent(uint32_t line);

      template <Screen::Size SIZE, Screen::Direction DIRECTION>
      void Scroll(uint32_t count);

      template <Screen::Location LOCATION>
      void ScrollTo(uint32_t line);

      template <Screen::Location SPECIFIC, Screen::Location ENDLOCATION>
      void ScrollTo(uint32_t line);

      template <Search::Skip SKIP>
      void ScrollToPlaylistSong(uint32_t count);

   private:
      void NextGotoMark(uint32_t count);
      void NextAddMark(uint32_t count);

      void AddMark(std::string const & input);
      void GotoMark(std::string const & input);

   private: //Alignment
      template <Screen::Direction DIRECTION>
      void Align(uint32_t count);

      template <Screen::Location LOCATION>
      void AlignTo(uint32_t line);

   private: //Quitting
      void Quit(uint32_t count);
      void QuitAll(uint32_t count);

   private: //Windows
      template <Screen::Skip SKIP, uint32_t OFFSET>
      void SetActiveWindow(uint32_t count);
      void ResetSelection(uint32_t count);

   private: //Editting
      template <int8_t OFFSET>
      void Move(uint32_t count);

   private:
      template <int SIGNAL>
      void SendSignal(uint32_t count);

      void ChangeMode(uint32_t count);

   private:
      void DisplayModeLine();
      std::string StateString();
      std::string ScrollString();

   private:
      typedef void (Ui::Normal::*ptrToMember)(uint32_t);
      typedef std::map<std::string, ptrToMember> ActionTable;

      struct KeyMapItem
      {
         KeyMapItem() :
            mode_  (Main::Vimpc::Normal),
            input_ (""),
            action_(NULL),
            count_ (0)
         {
         }

         Main::Vimpc::ModeName mode_;
         std::string           input_;
         ptrToMember           action_;
         uint32_t              count_;
      };

      typedef std::map<std::string, std::vector<KeyMapItem> > MapTable;
      typedef std::map<std::string, std::pair<uint32_t, uint32_t> > MarkTable;

   private:
      ModeWindow *     window_;
      std::string      input_;
      uint32_t         actionCount_;
      std::string      lastAction_;
      uint32_t         lastActionCount_;
      bool             wasSpecificCount_;
      bool             addMark_;
      bool             gotoMark_;

      ActionTable      actionTable_;
      MarkTable        markTable_;
      MapTable         mapTable_;
      MapNameTable     mapNames_;

      Main::Vimpc *    vimpc_;
      Ui::Search     & search_;
      Ui::Screen     & screen_;
      Mpc::Client    & client_;
      Mpc::Playlist  & playlist_;
      Main::Settings & settings_;
   };

}

#endif
/* vim: set sw=3 ts=3: */
