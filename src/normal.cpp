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

#include <iomanip>
#include <limits>
#include <sstream>

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
   settings_        (settings)
{
   // \todo figure out how to do alt + ctrl key combinations
   // for things like Ctrl+u and alt+1

   // \todo add proper handling of combination actions ie 'gt' and 'gg' etc
   // \todo display current count somewhere
   actionTable_['.']       = &Normal::RepeatLastAction;

   actionTable_['c']       = &Normal::ClearScreen;
   actionTable_['p']       = &Normal::Pause;
   //actionTable_['r']       = &Normal::Random; // \todo add back once i can be bothered toggling this properly
   actionTable_['s']       = &Normal::Stop;

   actionTable_['l']       = &Normal::SkipSong<Player::Next>;
   actionTable_['h']       = &Normal::SkipSong<Player::Previous>;
   actionTable_['x']       = &Normal::SkipArtist<Player::Next>;
   actionTable_['z']       = &Normal::SkipArtist<Player::Previous>;
   actionTable_['X']       = &Normal::SkipAlbum<Player::Next>;
   actionTable_['Z']       = &Normal::SkipAlbum<Player::Previous>;

   actionTable_['H']       = &Normal::Select<ScrollWindow::First>;
   actionTable_['M']       = &Normal::Select<ScrollWindow::Middle>;
   actionTable_['L']       = &Normal::Select<ScrollWindow::Last>;

   actionTable_['\n']      = &Normal::Confirm;
   actionTable_[KEY_ENTER] = &Normal::Confirm;

   actionTable_['N']       = &Normal::SearchResult<Search::Previous>;
   actionTable_['n']       = &Normal::SearchResult<Search::Next>;

   actionTable_['k']       = &Normal::Scroll<Screen::Line, Screen::Up>;
   actionTable_['j']       = &Normal::Scroll<Screen::Line, Screen::Down>;
   actionTable_[KEY_PPAGE] = &Normal::Scroll<Screen::Page, Screen::Up>;
   actionTable_[KEY_NPAGE] = &Normal::Scroll<Screen::Page, Screen::Down>;
   actionTable_[KEY_HOME]  = &Normal::ScrollTo<Screen::Top>;
   actionTable_['f']       = &Normal::ScrollTo<Screen::Current>;
   actionTable_[KEY_END]   = &Normal::ScrollTo<Screen::Bottom>;
   actionTable_['G']       = &Normal::ScrollTo<Screen::Specific, Screen::Bottom>;

   actionTable_[KEY_LEFT]  = actionTable_['h'];
   actionTable_[KEY_RIGHT] = actionTable_['l'];
   actionTable_[KEY_DOWN]  = actionTable_['j'];
   actionTable_[KEY_UP]    = actionTable_['k'];

   jumpTable_['g']         = &Normal::ScrollTo<Screen::Specific, Screen::Top>;
   jumpTable_['t']         = &Normal::SetActiveWindow<Screen::Next>;
   jumpTable_['T']         = &Normal::SetActiveWindow<Screen::Previous>;

   window_ = screen.CreateModeWindow();
}

Normal::~Normal()
{
   delete window_;
   window_ = NULL;
}

void Normal::Initialise(int input)
{
   actionCount_ = 0;
   DisplayModeLine();
   Refresh();
}

void Normal::Finalise(int input)
{
   Refresh();
}

void Normal::Refresh()
{
   window_->Print(1);
}

bool Normal::Handle(int input)
{
   static ActionTable * action = &actionTable_;

   // \todo work out how to handle 
   // ALT+<number> to change windows
   bool result = true;

   if ((input >= '0') && (input <= '9'))
   {
      uint64_t const newActionCount = ((static_cast<uint64_t>(actionCount_) * 10) + (input - '0'));

      if (newActionCount <= std::numeric_limits<uint32_t>::max())
      {
         actionCount_ = newActionCount;
      }
   }
   // \todo use a symbol
   else if (input == 27)
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

      DisplayModeLine();

      screen_.Update();
   }
   else if (input == 'g')
   {
      action = &jumpTable_;
   }
   else
   {
      action = &actionTable_;
   }

   return result;
}

bool Normal::CausesModeToStart(int input) const
{
   return ((input == '\n') || (input == 27));
}


bool Normal::Confirm(uint32_t count)
{
   screen_.Confirm();
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

void Normal::DisplayModeLine()
{
   // \todo need to display random, repeat, single, consume state somewhere

   std::ostringstream modeStream;
   
   float currentScroll = 0.0;

   if (screen_.PlaylistWindow().TotalNumberOfSongs() > 0)
   {
      currentScroll = ((screen_.PlaylistWindow().CurrentLine())/((float) screen_.PlaylistWindow().TotalNumberOfSongs()));
      currentScroll += .005;
      modeStream << (screen_.PlaylistWindow().CurrentLine() + 1) << "/" << screen_.PlaylistWindow().TotalNumberOfSongs() << " -- ";
   }

   if (screen_.PlaylistWindow().TotalNumberOfSongs() > screen_.MaxRows() - 1)
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
         modeStream << std::setw(2) << (int) (currentScroll * 100) << "%%";
      }
   }

   std::string currentState(client_.CurrentState() + "...");
   std::string modeLine(modeStream.str());
   std::string blankLine(screen_.MaxColumns() - (currentState.size()) - (modeLine.size() - 1), ' ');

   window_->SetLine("%s%s%s", currentState.c_str(),  blankLine.c_str(), modeLine.c_str());
}
