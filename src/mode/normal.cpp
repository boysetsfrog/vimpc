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
#include "window/error.hpp"
#include "window/songwindow.hpp"
#include "mode/command.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

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
   search_          (search),
   screen_          (screen),
   client_          (client),
   playlist_        (Main::Playlist()),
   settings_        (settings)
{
   // \todo this key bindings are pretty bad
   //       i will probably need to make them more specific
   //       to the tab/window or more modal or something

   // \todo display current count somewhere ?
   actionTable_["."]       = &Normal::RepeatLastAction;
   actionTable_["c"]       = &Normal::ClearScreen;

   // Player
   actionTable_["p"]       = &Normal::Pause;
   actionTable_["s"]       = &Normal::Stop;
   actionTable_["<BS>"]    = &Normal::Stop;

   actionTable_["C"]       = &Normal::Consume;
   actionTable_["R"]       = &Normal::Random;
   actionTable_["E"]       = &Normal::Repeat;
   actionTable_["S"]       = &Normal::Single;

   actionTable_["+"]       = &Normal::ChangeVolume<1>;
   actionTable_["-"]       = &Normal::ChangeVolume<-1>;

   // Console
   // \todo add an "insert" mode to console that just stays in command entry mode
   // This should really be implemented as Q as per vim (:he Ex-mode)
   //actionTable_['Q']       = &Normal::Insert;

   // Skipping
   actionTable_["I"]       = &Normal::SeekTo<Player::Start>;
   actionTable_[">"]       = &Normal::SkipSong<Player::Next>;
   actionTable_["<"]       = &Normal::SkipSong<Player::Previous>;
   actionTable_["]"]       = &Normal::SkipArtist<Player::Next>;
   actionTable_["["]       = &Normal::SkipArtist<Player::Previous>;
   actionTable_["}"]       = &Normal::SkipAlbum<Player::Next>;
   actionTable_["{"]       = &Normal::SkipAlbum<Player::Previous>;

   // Selection
   actionTable_["H"]       = &Normal::Select<ScrollWindow::First>;
   actionTable_["M"]       = &Normal::Select<ScrollWindow::Middle>;
   actionTable_["L"]       = &Normal::Select<ScrollWindow::Last>;

   // Playlist
   actionTable_["d"]       = &Normal::DeleteSong<Mpc::Song::Single>;
   actionTable_["D"]       = &Normal::DeleteSong<Mpc::Song::All>;
   actionTable_["a"]       = &Normal::AddSong<Mpc::Song::Single>;
   actionTable_["A"]       = &Normal::AddSong<Mpc::Song::All>;
   actionTable_["x"]       = &Normal::CropSong<Mpc::Song::Single>;
   actionTable_["X"]       = &Normal::CropSong<Mpc::Song::All>;

   actionTable_["<Del>"]   = &Normal::DeleteSong<Mpc::Song::Single>;

   // ! \todo this is a bit dodgy, is there a better key for this?
   //         we need a paste above and paste below
   actionTable_["P"]       = &Normal::PasteBuffer;

   // Navigation
   actionTable_["l"]       = &Normal::Right;
   actionTable_["h"]       = &Normal::Left;
   actionTable_["\n"]      = &Normal::Confirm;
   actionTable_["<Return>"]= &Normal::Confirm;
   actionTable_["<C-J>"]   = &Normal::Confirm;
   actionTable_["<Enter>"] = &Normal::Confirm;

   // Searching
   actionTable_["N"]       = &Normal::SearchResult<Search::Previous>;
   actionTable_["n"]       = &Normal::SearchResult<Search::Next>;

   // Scrolling
   actionTable_["k"]       = &Normal::Scroll<Screen::Line, Screen::Up>;
   actionTable_["j"]       = &Normal::Scroll<Screen::Line, Screen::Down>;
   actionTable_["<PageUp>"]   = &Normal::Scroll<Screen::Page, Screen::Up>;
   actionTable_["<PageDown>"] = &Normal::Scroll<Screen::Page, Screen::Down>;
   actionTable_["<C-U>"]   = &Normal::Scroll<Screen::Page, Screen::Up>;
   actionTable_["<C-D>"]   = &Normal::Scroll<Screen::Page, Screen::Down>;
   actionTable_["<C-B>"]   = &Normal::Scroll<Screen::FullPage, Screen::Up>;
   actionTable_["<C-F>"]   = &Normal::Scroll<Screen::FullPage, Screen::Down>;
   actionTable_["<C-Y>"]   = &Normal::Align<Screen::Up>;
   actionTable_["<C-E>"]   = &Normal::Align<Screen::Down>;
   actionTable_["<Home>"]  = &Normal::ScrollTo<Screen::Top>;
   actionTable_["f"]       = &Normal::ScrollToCurrent<1>;
   actionTable_["F"]       = &Normal::ScrollToCurrent<-1>;
   actionTable_["<End>"]   = &Normal::ScrollTo<Screen::Bottom>;
   actionTable_["G"]       = &Normal::ScrollTo<Screen::Specific, Screen::Bottom>;

   actionTable_["<C-Z>"]   = &Normal::SendSignal<SIGTSTP>;
   actionTable_["<C-C>"]   = &Normal::SendSignal<SIGINT>;

   // Editting
   actionTable_["<C-A>"]   = &Normal::Move<1>;
   actionTable_["<C-X>"]   = &Normal::Move<-1>;

   //
   actionTable_["<Left>"]  = actionTable_["h"];
   actionTable_["<Right>"] = actionTable_["l"];
   actionTable_["<Down>"]  = actionTable_["j"];
   actionTable_["<Up>"]    = actionTable_["k"];

   //
   actionTable_["e"]       = &Normal::Edit;
   actionTable_["v"]       = &Normal::Visual;

   // Library
   actionTable_["o"]       = &Normal::Expand;
   actionTable_["u"]       = &Normal::Collapse;

   // Jumping
   actionTable_["gg"]      = &Normal::ScrollTo<Screen::Specific, Screen::Top>;
   actionTable_["gf"]      = &Normal::ScrollToCurrent<1>;
   actionTable_["gF"]      = &Normal::ScrollToCurrent<-1>;
   actionTable_["gp"]      = &Normal::ScrollToPlaylistSong<Search::Next>;
   actionTable_["gP"]      = &Normal::ScrollToPlaylistSong<Search::Previous>;
   actionTable_["gt"]      = &Normal::SetActiveWindow<Screen::Next, 0>;
   actionTable_["gT"]      = &Normal::SetActiveWindow<Screen::Previous, 0>;

   // Align the text to a location on the screen
   actionTable_["z."]        = &Normal::AlignTo<Screen::Centre>;
   actionTable_["z<Enter>"]  = &Normal::AlignTo<Screen::Top>;
   actionTable_["z<Return>"] = &Normal::AlignTo<Screen::Top>;
   actionTable_["z-"]        = &Normal::AlignTo<Screen::Bottom>;

   // Support for Z{Z,Q}
   actionTable_["ZZ"]        = &Normal::Quit;
   actionTable_["ZQ"]        = &Normal::QuitAll;

   // Alt key combos
   actionTable_["<A-1>"]     = &Normal::SetActiveWindow<Screen::Absolute, 0>;
   actionTable_["<A-2>"]     = &Normal::SetActiveWindow<Screen::Absolute, 1>;
   actionTable_["<A-3>"]     = &Normal::SetActiveWindow<Screen::Absolute, 2>;
   actionTable_["<A-4>"]     = &Normal::SetActiveWindow<Screen::Absolute, 3>;
   actionTable_["<A-5>"]     = &Normal::SetActiveWindow<Screen::Absolute, 4>;
   actionTable_["<A-6>"]     = &Normal::SetActiveWindow<Screen::Absolute, 5>;
   actionTable_["<A-7>"]     = &Normal::SetActiveWindow<Screen::Absolute, 6>;
   actionTable_["<A-8>"]     = &Normal::SetActiveWindow<Screen::Absolute, 7>;
   actionTable_["<A-9>"]     = &Normal::SetActiveWindow<Screen::Absolute, 8>;

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
   input_ = "";
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

   if ((input >= '0') && (input <= '9') && ((input & (1 << 31)) == 0))
   {
      uint64_t const newActionCount = ((static_cast<uint64_t>(actionCount_) * 10) + (input - '0'));

      if (newActionCount <= std::numeric_limits<uint32_t>::max())
      {
         actionCount_ = newActionCount;
      }
   }
   else if (input == ESCAPE_KEY)
   {
      input_       = "";
      actionCount_ = 0;

      screen_.ActiveWindow().Escape();
   }
   else
   {
      wasSpecificCount_ = (actionCount_ != 0);

      uint32_t count = (actionCount_ > 0) ? actionCount_ : 1;

      if (input != '.')
      {
         lastAction_      = input;
         lastActionCount_ = actionCount_;
      }

      // Todo use a function that turns characters into strings, required for pageup etc
      input_ += InputCharToString(input);

      int         foundCount = 0;
      ptrToMember actionFunc;

      for (ActionTable::const_iterator it = actionTable_.begin(); it != actionTable_.end(); ++it)
      {
         if (it->first.substr(0, input_.size()) == input_)
         {
            foundCount++;
            actionFunc = it->second;
         }
      }

      if (foundCount == 1)
      {
         (*this.*actionFunc)(count);
         actionCount_ = 0;
         input_       = "";
      }
      else if (foundCount == 0)
      {
         actionCount_ = 0;
         input_       = "";
      }

      action = &actionTable_;
   }

   return result;
}

bool Normal::CausesModeToStart(int input) const
{
   return ((input == '\n') || (input == ESCAPE_KEY));
}

bool Normal::CausesModeToEnd(int input) const
{
   return false;
}

std::string Normal::InputCharToString(int input) const
{
   static std::map<int, std::string> conversionTable;

   std::string result    = "";
   bool        converted = false;

   if (conversionTable.size() == 0)
   {
      conversionTable[KEY_PPAGE]     = "PageUp";
      conversionTable[KEY_NPAGE]     = "PageDown";
      conversionTable[KEY_HOME]      = "Home";
      conversionTable[KEY_END]       = "End";
      conversionTable[KEY_LEFT]      = "Left";
      conversionTable[KEY_RIGHT]     = "Right";
      conversionTable[KEY_DOWN]      = "Down";
      conversionTable[KEY_UP]        = "Up";
      conversionTable[KEY_DC]        = "Del";
      conversionTable[KEY_BACKSPACE] = "BS";
      conversionTable[0x7F]          = "BS";
      conversionTable[KEY_ENTER]     = "Enter";
      conversionTable['\n']          = "Return";
   }

   std::map<int, std::string>::const_iterator it = conversionTable.find(input);

   if ((it == conversionTable.end()) && ((input & (1 << 31)) != 0))
   {
      input     = (input & 0x7FFFFFFF);
      converted = true;
      result    = "A-";
   }

   if ((it == conversionTable.end()) && ((input <= 27) && (input >= 1)))
   {
      input     = 'A' + input - 1;
      converted = true;
      result    = "C-";
   }

   if (it != conversionTable.end())
   {
      converted = true;
      result += it->second;
   }
   else
   {
      result += char (input);
   }

   if (converted == true)
   {
      result = "<" + result + ">";
      //printf("The result %s\n", result.c_str());
   }

   return result;
}


void Normal::ClearScreen(uint32_t count)
{
   Player::ClearScreen();
}

void Normal::Pause(uint32_t count)
{
   Player::Pause();
}

void Normal::Stop(uint32_t count)
{
   Player::Stop();
}


void Normal::Consume(uint32_t count)
{
   Player::ToggleConsume();
}

void Normal::Random(uint32_t count)
{
   Player::ToggleRandom();
}

void Normal::Repeat(uint32_t count)
{
   Player::ToggleRepeat();
}

void Normal::Single(uint32_t count)
{
   Player::ToggleSingle();
}


template <int Delta>
void Normal::ChangeVolume(uint32_t count)
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

   Player::Volume(CurrentVolume);
}

template <Ui::Player::Location LOCATION>
void Normal::SeekTo(uint32_t count)
{
   if (LOCATION == Player::Start)
   {
      client_.SeekTo(0);
   }
   else if (LOCATION == Player::End)
   {
      Player::SkipSong(Player::Next, count);
   }
}

void Normal::Left(uint32_t count)
{
   screen_.ActiveWindow().Left(*this, count);
}

void Normal::Right(uint32_t count)
{
   screen_.ActiveWindow().Right(*this, count);
}

void Normal::Confirm(uint32_t count)
{
   screen_.ActiveWindow().Confirm();
}

void Normal::RepeatLastAction(uint32_t count)
{
   actionCount_ = (actionCount_ > 0) ? count : lastActionCount_;

   if (lastAction_ != 0)
   {
      Handle(lastAction_);
   }
}

void Normal::Expand(uint32_t count)
{
   if (screen_.ActiveWindow().CurrentLine() < Main::Library().Size())
   {
      Main::Library().Expand(screen_.ActiveWindow().CurrentLine());
   }
}

void Normal::Collapse(uint32_t count)
{
   if (screen_.ActiveWindow().CurrentLine() < Main::Library().Size())
   {
      Main::Library().Collapse(screen_.ActiveWindow().CurrentLine());
   }
}


void Normal::Edit(uint32_t count)
{
   screen_.ActiveWindow().Edit();
}

void Normal::Visual(uint32_t count)
{
   screen_.ActiveWindow().Visual();
}


template <Mpc::Song::SongCollection COLLECTION>
void Normal::AddSong(uint32_t count)
{
   if (COLLECTION == Mpc::Song::All)
   {
      screen_.ActiveWindow().AddAllLines();
   }
   else if (COLLECTION == Mpc::Song::Single)
   {
      screen_.ActiveWindow().AddLine(screen_.ActiveWindow().CurrentLine(), count);
   }
}

template <Mpc::Song::SongCollection COLLECTION>
void Normal::DeleteSong(uint32_t count)
{
   //! \todo Make delete and add take a movement operation?
   //!       ie to do stuff like dG, this may require making some kind of movement
   //!          table or something rather than the way it currently works
   if (COLLECTION == Mpc::Song::All)
   {
      screen_.ActiveWindow().DeleteAllLines();
   }
   else if (COLLECTION == Mpc::Song::Single)
   {
      screen_.ActiveWindow().DeleteLine(screen_.ActiveWindow().CurrentLine(), count);
   }
}

template <Mpc::Song::SongCollection COLLECTION>
void Normal::CropSong(uint32_t count)
{
   if (COLLECTION == Mpc::Song::All)
   {
      screen_.ActiveWindow().CropAllLines();
   }
   else if (COLLECTION == Mpc::Song::Single)
   {
      screen_.ActiveWindow().CropLine(screen_.ActiveWindow().CurrentLine(), count);
   }
}

void Normal::PasteBuffer(uint32_t count)
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
}


//Implementation of selecting functions
template <ScrollWindow::Position POSITION>
void Normal::Select(uint32_t count)
{
   screen_.Select(POSITION, count);
}


//Implementation of searching functions
template <Ui::Search::Skip SKIP>
void Normal::SearchResult(uint32_t count)
{
   search_.SearchResult(SKIP, count);
}


//Implementation of skipping functions
template <Ui::Player::Skip SKIP>
void Normal::SkipSong(uint32_t count)
{
   Player::SkipSong(SKIP, count);
}

template <Ui::Player::Skip SKIP>
void Normal::SkipAlbum(uint32_t count)
{
   Player::SkipAlbum(SKIP, count);
}

template <Ui::Player::Skip SKIP>
void Normal::SkipArtist(uint32_t count)
{
   Player::SkipArtist(SKIP, count);
}


// Implementation of scrolling functions
template <int8_t OFFSET>
void Normal::ScrollToCurrent(uint32_t line)
{
   screen_.ScrollTo(Screen::Current, (wasSpecificCount_ == true) ? line * OFFSET : 0);
}

template <Screen::Size SIZE, Screen::Direction DIRECTION>
void Normal::Scroll(uint32_t count)
{
   screen_.Scroll(SIZE, DIRECTION, count);
}

template <Screen::Location LOCATION>
void Normal::ScrollTo(uint32_t line)
{
   screen_.ScrollTo(LOCATION);
}

template <Screen::Location SPECIFIC, Screen::Location ENDLOCATION>
void Normal::ScrollTo(uint32_t line)
{
   if ((SPECIFIC == Screen::Specific) && (wasSpecificCount_ == false))
   {
      ScrollTo<ENDLOCATION>(line);
   }
   else
   {
      screen_.ScrollTo(SPECIFIC, line);
   }
}


template <Search::Skip SKIP>
void Normal::ScrollToPlaylistSong(uint32_t count)
{
   if (SKIP == Search::Previous)
   {
      screen_.ScrollTo(screen_.ActiveWindow().Playlist(-1 * count));
   }
   else
   {
      screen_.ScrollTo(screen_.ActiveWindow().Playlist(count));
   }
}


template <Screen::Direction DIRECTION>
void Normal::Align(uint32_t count)
{
   screen_.Align(DIRECTION, count);
}

template <Screen::Location LOCATION>
void Normal::AlignTo(uint32_t line)
{
   if (wasSpecificCount_ == false)
   {
      line = 0;
   }

   screen_.AlignTo(LOCATION, line);
}


// Implementation of window functions
template <Screen::Skip SKIP, uint32_t OFFSET>
void Normal::SetActiveWindow(uint32_t count)
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
}

// Proxy for quit commands
void Normal::Quit(uint32_t count)
{
   QuitAll(0);
}

void Normal::QuitAll(uint32_t count)
{
   if (settings_.StopOnQuit() == true)
   {
      Player::Stop();
   }

   Player::Quit();
}

// Implementation of editting functions
template <int8_t OFFSET>
void Normal::Move(uint32_t count)
{
   if (screen_.GetActiveWindow() == Screen::Playlist)
   {
      uint32_t const currentLine = screen_.ActiveWindow().CurrentLine();
      int32_t position = currentLine + (count * OFFSET);

      if (position >= static_cast<int32_t>(screen_.ActiveWindow().ContentSize()))
      {
         position = screen_.ActiveWindow().ContentSize();
      }
      else if (position <= 0)
      {
         position = 0;
      }

      client_.Move(currentLine, position);

      Mpc::Song * song = Main::Playlist().Get(currentLine);
      Main::Playlist().Remove(currentLine, 1);
      Main::Playlist().Add(song, position);
      screen_.ActiveWindow().ScrollTo(position);
      screen_.Update();
   }
}


template <int SIGNAL>
void Normal::SendSignal(uint32_t count)
{
   kill(getpid(), SIGNAL);
}


void Normal::DisplayModeLine()
{
   std::ostringstream modeStream;

   float currentScroll = 0.0;

   if (screen_.ActiveWindow().ContentSize() > 0)
   {
      currentScroll = ((screen_.ActiveWindow().CurrentLine())/(static_cast<float>(screen_.ActiveWindow().ContentSize()) - 1));
      currentScroll += .005;
      modeStream << (screen_.ActiveWindow().CurrentLine() + 1) << "/" << (screen_.ActiveWindow().ContentSize() + 1) << " -- ";
   }

   if (screen_.ActiveWindow().ContentSize() > static_cast<int32_t>(screen_.MaxRows()) - 1)
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
      volume += " [Volume: " + std::string(vol) + "%]";
   }

   std::string currentState("[State: " + client_.CurrentState() + "]" + volume + toggles);

   std::string modeLine(modeStream.str());

   int32_t const WhiteSpaceLength = screen_.MaxColumns() - (currentState.size()) - (modeLine.size() - 1);

   std::string blankLine("");

   if (WhiteSpaceLength > 0)
   {
      blankLine = std::string(screen_.MaxColumns() - (currentState.size()) - (modeLine.size() - 1), ' ');
   }

   window_->SetLine("%s%s%s", currentState.c_str(),  blankLine.c_str(), modeLine.c_str());
}
/* vim: set sw=3 ts=3: */
