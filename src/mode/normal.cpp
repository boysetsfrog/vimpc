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

   normal.cpp - normal mode input handling
   */

#include "normal.hpp"

#include "mpdclient.hpp"
#include "vimpc.hpp"
#include "buffer/library.hpp"
#include "buffer/playlist.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

#define ESCAPE_KEY 27

using namespace Ui;

Normal::Normal(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings, Ui::Search & search) :
   Player           (screen, client, settings),
   window_          (NULL),
   actionCount_     (0),
   lastAction_      (0),
   lastActionCount_ (0),
   wasSpecificCount_(false),
   actionTable_     (),
   jumpTable_       (),
   alignTable_      (),
   search_          (search),
   screen_          (screen),
   client_          (client),
   playlist_        (Main::Playlist()),
   settings_        (settings)
{
   // \todo display current count somewhere ?
   actionTable_['.']       = &Normal::RepeatLastAction;
   actionTable_['c']       = &Normal::ClearScreen;

   // Player
   actionTable_['p']           = &Normal::Pause;
   actionTable_['r']           = &Normal::Random;
   actionTable_['S']           = &Normal::Single;
   actionTable_['s']           = &Normal::Stop;
   actionTable_[KEY_BACKSPACE] = &Normal::Stop;

   actionTable_['+']       = &Normal::ChangeVolume<1>;
   actionTable_['-']       = &Normal::ChangeVolume<-1>;

   // Console
   // \todo add an "insert" mode to console that just stays in command entry mode
   // This should really be implemented as Q as per vim (:he Ex-mode)
   //actionTable_['Q']       = &Normal::Insert;

   //! \todo make it so these can be used to navigate the library
   // Skipping
   actionTable_['>']       = &Normal::SkipSong<Player::Next>;
   actionTable_['<']       = &Normal::SkipSong<Player::Previous>;
   actionTable_['w']       = &Normal::SkipArtist<Player::Next>;
   actionTable_['q']       = &Normal::SkipArtist<Player::Previous>;
   actionTable_['W']       = &Normal::SkipAlbum<Player::Next>;
   actionTable_['Q']       = &Normal::SkipAlbum<Player::Previous>;

   // Selection
   actionTable_['H']       = &Normal::Select<ScrollWindow::First>;
   actionTable_['M']       = &Normal::Select<ScrollWindow::Middle>;
   actionTable_['L']       = &Normal::Select<ScrollWindow::Last>;

   // Playlist
   // ! \todo these should only work if the current window is the correct one
   actionTable_['d']       = &Normal::DeleteSong<Mpc::Song::Single>;
   actionTable_['D']       = &Normal::DeleteSong<Mpc::Song::All>;
   actionTable_['a']       = &Normal::AddSong<Mpc::Song::Single>;
   actionTable_['A']       = &Normal::AddSong<Mpc::Song::All>;
   actionTable_['P']       = &Normal::PasteBuffer;

   // Navigation
   actionTable_['l']       = &Normal::Right;
   actionTable_['h']       = &Normal::Left;
   actionTable_['\n']      = &Normal::Confirm;
   actionTable_[KEY_ENTER] = &Normal::Confirm;

   // Searching
   actionTable_['N']       = &Normal::SearchResult<Search::Previous>;
   actionTable_['n']       = &Normal::SearchResult<Search::Next>;

   // Scrolling
   actionTable_['k']       = &Normal::Scroll<Screen::Line, Screen::Up>;
   actionTable_['j']       = &Normal::Scroll<Screen::Line, Screen::Down>;
   actionTable_[KEY_PPAGE] = &Normal::Scroll<Screen::Page, Screen::Up>;
   actionTable_[KEY_NPAGE] = &Normal::Scroll<Screen::Page, Screen::Down>;
   actionTable_['U'+1 - 'A'] = &Normal::Scroll<Screen::Page, Screen::Up>; //CTRL + U
   actionTable_['D'+1 - 'A'] = &Normal::Scroll<Screen::Page, Screen::Down>; //CTRL + D
   actionTable_['Y'+1 - 'A'] = &Normal::Align<Screen::Up>; //CTRL + Y
   actionTable_['E'+1 - 'A'] = &Normal::Align<Screen::Down>; //CTRL + E
   actionTable_[KEY_HOME]  = &Normal::ScrollTo<Screen::Top>;
   actionTable_['f']       = &Normal::ScrollTo<Screen::Current>;
   actionTable_['e']       = &Normal::ScrollTo<Screen::PlaylistNext>;
   actionTable_['E']       = &Normal::ScrollTo<Screen::PlaylistPrev>;
   actionTable_[KEY_END]   = &Normal::ScrollTo<Screen::Bottom>;
   actionTable_['G']       = &Normal::ScrollTo<Screen::Specific, Screen::Bottom>;

   //
   actionTable_[KEY_LEFT]  = actionTable_['h'];
   actionTable_[KEY_RIGHT] = actionTable_['l'];
   actionTable_[KEY_DOWN]  = actionTable_['j'];
   actionTable_[KEY_UP]    = actionTable_['k'];

   // Library
   actionTable_['o']       = &Normal::Expand;
   actionTable_['u']       = &Normal::Collapse;

   // Jumping
   jumpTable_['g']         = &Normal::ScrollTo<Screen::Specific, Screen::Top>;
   jumpTable_['t']         = &Normal::SetActiveWindow<Screen::Next, 0>;
   jumpTable_['T']         = &Normal::SetActiveWindow<Screen::Previous, 0>;

   // Align the text to a location on the screen
   // \todo this should only work for selectwindows
   alignTable_['.']        = &Normal::AlignTo<Screen::Centre>;
   alignTable_['\n']       = &Normal::AlignTo<Screen::Top>;
   alignTable_['-']        = &Normal::AlignTo<Screen::Bottom>;

   escapeTable_['1']       = &Normal::SetActiveWindow<Screen::Absolute, 0>;
   escapeTable_['2']       = &Normal::SetActiveWindow<Screen::Absolute, 1>;
   escapeTable_['3']       = &Normal::SetActiveWindow<Screen::Absolute, 2>;
   escapeTable_['4']       = &Normal::SetActiveWindow<Screen::Absolute, 3>;
   escapeTable_['5']       = &Normal::SetActiveWindow<Screen::Absolute, 4>;
   escapeTable_['6']       = &Normal::SetActiveWindow<Screen::Absolute, 5>;
   escapeTable_['7']       = &Normal::SetActiveWindow<Screen::Absolute, 6>;
   escapeTable_['8']       = &Normal::SetActiveWindow<Screen::Absolute, 7>;
   escapeTable_['9']       = &Normal::SetActiveWindow<Screen::Absolute, 8>;

   window_ = screen.CreateModeWindow();
}

Normal::~Normal()
{
   screen_.DeleteModeWindow(window_);
   window_ = NULL;
}

void Normal::Initialise(int input)
{
   actionCount_ = 0;
   Refresh();
}

void Normal::Finalise(int input)
{
   window_->Print(0);
}

void Normal::Refresh()
{
   DisplayModeLine();
   window_->Print(0);
}

bool Normal::Handle(int input)
{
   static ActionTable * action = &actionTable_;

   bool result = true;

   if ((input & (1 << 31)) != 0)
   {
      input  = (input & 0x7FFFFFFF);
      action = &escapeTable_;
   }

   if ((input >= '0') && (input <= '9') && (action != &escapeTable_))
   {
      uint64_t const newActionCount = ((static_cast<uint64_t>(actionCount_) * 10) + (input - '0'));

      if (newActionCount <= std::numeric_limits<uint32_t>::max())
      {
         actionCount_ = newActionCount;
      }
   }
   else if (input == ESCAPE_KEY)
   {
      action       = &actionTable_;
      actionCount_ = 0;
   }
   else if (action->find(input) != action->end())
   {
      wasSpecificCount_ = (actionCount_ != 0);

      uint32_t count = (actionCount_ > 0) ? actionCount_ : 1;

      if (input != '.')
      {
         lastAction_      = input;
         lastActionCount_ = actionCount_;
      }

      ptrToMember actionFunc = (*action)[input];
      result = (*this.*actionFunc)(count);
      actionCount_ = 0;

      action = &actionTable_;
   }
   else if (input == 'g')
   {
      action = &jumpTable_;
   }
   else if (input == 'z')
   {
      action = &alignTable_;
   }
   else
   {
      action = &actionTable_;
   }

   return result;
}

bool Normal::CausesModeToStart(int input) const
{
   return ((input == '\n') || (input == ESCAPE_KEY));
}


bool Normal::ClearScreen(uint32_t count)
{
   return Player::ClearScreen();
}

bool Normal::Pause(uint32_t count)
{
   return Player::Pause();
}

bool Normal::Random(uint32_t count)
{
   return Player::ToggleRandom();
}

bool Normal::Single(uint32_t count)
{
   return Player::ToggleSingle();
}

bool Normal::Stop(uint32_t count)
{
   return Player::Stop();
}


template <int Delta>
bool Normal::ChangeVolume(uint32_t count)
{
   int CurrentVolume = client_.Volume() + (count * Delta);

   if (CurrentVolume < 0)
   {
      CurrentVolume = 0;
   }
   else if (CurrentVolume > 100)
   {
      CurrentVolume = 100;
   }

   return Player::Volume(CurrentVolume);
}


bool Normal::Left(uint32_t count)
{
   screen_.ActiveWindow().Left(*this, count);
   return true;
}

bool Normal::Right(uint32_t count)
{
   screen_.ActiveWindow().Right(*this, count);
   return true;
}

bool Normal::Confirm(uint32_t count)
{
   screen_.ActiveWindow().Confirm();
   return true;
}

bool Normal::RepeatLastAction(uint32_t count)
{
   actionCount_ = (actionCount_ > 0) ? count : lastActionCount_;

   if (lastAction_ != 0)
   {
      Handle(lastAction_);
   }

   return true;
}

bool Normal::Expand(uint32_t count)
{
   if (screen_.ActiveWindow().CurrentLine() < Main::Library().Size())
   {
      Main::Library().Expand(screen_.ActiveWindow().CurrentLine());
   }

   return true;
}

bool Normal::Collapse(uint32_t count)
{
   if (screen_.ActiveWindow().CurrentLine() < Main::Library().Size())
   {
      Main::Library().Collapse(screen_.ActiveWindow().CurrentLine());
   }

   return true;
}

// Implementation of library actions
//
// \todo this should be implemented using the window somehow
template <Mpc::Song::SongCollection COLLECTION>
bool Normal::AddSong(uint32_t count)
{
   if (COLLECTION == Mpc::Song::All)
   {
      client_.AddAllSongs();
      screen_.ScrollTo(screen_.ActiveWindow().CurrentLine());
   }
   else
   {
      if (count > 1)
      {
         client_.StartCommandList();
      }

      for (uint32_t i = 0; i < count; ++i)
      {
         if (screen_.GetActiveWindow() == Screen::Library)
         {
            Main::Library().AddToPlaylist(COLLECTION, client_, screen_.ActiveWindow().CurrentLine() + i);
         }
         else if (screen_.GetActiveWindow() == Screen::Browse)
         {
            Main::Browse().AddToPlaylist(client_, screen_.ActiveWindow().CurrentLine() + i);
         }
      }

      if (count > 1)
      {
         client_.SendCommandList();
      }

      if (screen_.GetActiveWindow() != Screen::Playlist)
      {
         screen_.ActiveWindow().Scroll(count);
      }
   }

   return true;
}

template <Mpc::Song::SongCollection COLLECTION>
bool Normal::DeleteSong(uint32_t count)
{
   //! \todo should probably move this into a function on each window
   //! rather than having it in here and having to check what the window is

   //! \todo Make delete and add take a movement operation?
   //!       ie to do stuff like dG, this may require making some kind of movement
   //!          table or something rather than the way it currently works

   //! \todo it seems like this needs to know a lot of stuff, surely i could abstract this out?
   if ((screen_.GetActiveWindow() == Screen::Playlist) ||
       (screen_.GetActiveWindow() == Screen::Browse) ||
       (COLLECTION == Mpc::Song::All))
   {
      uint32_t const currentLine = screen_.ActiveWindow().CurrentLine();

      Main::PlaylistPasteBuffer().Clear();

      if (COLLECTION == Mpc::Song::Single)
      {
         int32_t index = currentLine;

         if ((screen_.GetActiveWindow() == Screen::Browse))
         {
            client_.StartCommandList();

            for (uint32_t i = 0; i < count; ++i)
            {
               int32_t index = currentLine;

               index = Main::Playlist().Index(Main::Browse().Get(index + i));
               screen_.ActiveWindow().Scroll(1);

               if (index >= 0)
               {
                  client_.Delete(index);
                  playlist_.Remove(index, 1);
               }
            }

            client_.SendCommandList();
         }
         else if (index >= 0)
         {
            client_.Delete(index, count + index);
            playlist_.Remove(index, count);
         }
      }
      else if (COLLECTION == Mpc::Song::All)
      {
         Main::PlaylistPasteBuffer().Clear();
         client_.Clear();
         playlist_.Clear();
      }

      if ((screen_.GetActiveWindow() != Screen::Browse))
      {
         screen_.ScrollTo(currentLine);
      }
   }
   else if (screen_.GetActiveWindow() == Screen::Lists)
   {
      uint32_t const currentLine = screen_.ActiveWindow().CurrentLine();

      client_.RemovePlaylist(Main::Lists().Get(currentLine));
   }

   return true;
}

bool Normal::PasteBuffer(uint32_t count)
{
   uint32_t position = 0;

   client_.StartCommandList();

   for (uint32_t i = 0; i < count; ++i)
   {
      for (uint32_t j = 0; j < Main::PlaylistPasteBuffer().Size(); ++j)
      {
         client_.Add(*Main::PlaylistPasteBuffer().Get(j), screen_.ActiveWindow().CurrentLine() + position);
         Main::Playlist().Add(Main::PlaylistPasteBuffer().Get(j), screen_.ActiveWindow().CurrentLine() + position);

         position++;
      }
   }

   client_.SendCommandList();

   return true;
}


//Implementation of selecting functions
template <ScrollWindow::Position POSITION>
bool Normal::Select(uint32_t count)
{
   screen_.Select(POSITION, count);
   return true;
}


//Implementation of searching functions
template <Ui::Search::Skip SKIP>
bool Normal::SearchResult(uint32_t count)
{
   return search_.SearchResult(SKIP, count);
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


// Implementation of scrolling functions
template <Screen::Size SIZE, Screen::Direction DIRECTION>
bool Normal::Scroll(uint32_t count)
{
   screen_.Scroll(SIZE, DIRECTION, count);
   return true;
}

template <Screen::Location LOCATION>
bool Normal::ScrollTo(uint32_t line)
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


template <Screen::Direction DIRECTION>
bool Normal::Align(uint32_t line)
{
   screen_.Align(DIRECTION, line);

   return true;
}

template <Screen::Location LOCATION>
bool Normal::AlignTo(uint32_t line)
{
   if (wasSpecificCount_ == false)
   {
      line = 0;
   }

   screen_.AlignTo(LOCATION, line);

   return true;
}


// Implementation of window functions
template <Screen::Skip SKIP, uint32_t OFFSET>
bool Normal::SetActiveWindow(uint32_t count)
{
   if (SKIP == Screen::Absolute)
   {
      screen_.SetActiveWindow(static_cast<Screen::MainWindow>(OFFSET));
   }
   else if ((SKIP == Screen::Next) && (wasSpecificCount_ == true))
   {
      screen_.SetActiveWindow(static_cast<Screen::MainWindow>(count - 1));
   }
   else if ((SKIP == Screen::Previous) && (wasSpecificCount_ == true))
   {
      count = (count % screen_.VisibleWindows());

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


void Normal::DisplayModeLine()
{
   // \todo need to display random, repeat, single, consume state somewhere
   std::ostringstream modeStream;

   float currentScroll = 0.0;

   if (playlist_.Size() > 0)
   {
      //! \todo should make work for ac
      currentScroll = ((screen_.ActiveWindow().CurrentLine())/(static_cast<float>(screen_.ActiveWindow().ContentSize()) - 1));
      currentScroll += .005;
      modeStream << (screen_.ActiveWindow().CurrentLine() + 1) << "/" << (screen_.ActiveWindow().ContentSize() + 1) << " -- ";
   }

   if (playlist_.Size() > screen_.MaxRows() - 1)
   {
      if (currentScroll <= .010)
      {
         modeStream << "Top ";
      }
      else if (currentScroll >= 1.0)
      {
         modeStream << "Bot ";
      }
      else
      {
         modeStream << std::setw(2) << static_cast<int>(currentScroll * 100) << "%%";
      }
   }

   std::string toggles = "";
   std::string random  = (client_.Random() == true) ? "random, " : "";
   std::string repeat  = (client_.Repeat() == true) ? "repeat, " : "";
   std::string single  = (client_.Single() == true) ? "single, " : "";
   std::string consume = (client_.Consume() == true) ? "consume, " : "";

   if ((random != "") || (repeat != "") || (single != "") || (consume != ""))
   {
      toggles += " [ON: ";
      toggles += random + repeat + single + consume;
      toggles = toggles.substr(0, toggles.length() - 2);
      toggles += "]";
   }


   std::string volume = "";

   if (client_.Volume() != -1)
   {
      char vol[8];
      snprintf(vol, 8, "%d", client_.Volume());
      volume += " [Volume: " + std::string(vol) + "]";
   }

   std::string currentState("[State: " + client_.CurrentState() + "]" + volume + toggles);

   std::string modeLine(modeStream.str());
   std::string blankLine(screen_.MaxColumns() - (currentState.size()) - (modeLine.size() - 1), ' ');

   window_->SetLine("%s%s%s", currentState.c_str(),  blankLine.c_str(), modeLine.c_str());
}
