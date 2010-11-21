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

#include "vimpc.hpp"

#include <limits>

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
   settings_        (settings)
{
   // \todo figure out how to do alt + ctrl key combinations
   // for things like Ctrl+u and alt+1

   // \todo add proper handling of combination actions ie 'gt' and 'gg' etc
   // \todo display current count somewhere
   actionTable_['.']       = &Normal::RepeatLastAction;

   actionTable_['c']       = &Normal::ClearScreen;
   actionTable_['p']       = &Normal::Pause;
   actionTable_['r']       = &Normal::Random;
   actionTable_['s']       = &Normal::Stop;

   actionTable_['l']       = &Normal::SkipSong<Player::Next>;
   actionTable_['h']       = &Normal::SkipSong<Player::Previous>;
   actionTable_['x']       = &Normal::SkipArtist<Player::Next>;
   actionTable_['z']       = &Normal::SkipArtist<Player::Previous>;

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
   actionTable_['G']       = &Normal::ScrollTo<Screen::Specific>;

   actionTable_[KEY_LEFT]  = actionTable_['h'];
   actionTable_[KEY_RIGHT] = actionTable_['l'];
   actionTable_[KEY_DOWN]  = actionTable_['j'];
   actionTable_[KEY_UP]    = actionTable_['k'];

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

   window_->SetLine("");
}

void Normal::Finalise(int input)
{
}

bool Normal::Handle(int input)
{
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
      actionCount_ = 0;
   }

   else if (actionTable_.find(input) != actionTable_.end())
   {
      wasSpecificCount_ = (actionCount_ != 0);

      uint32_t count = (actionCount_ > 0) ? actionCount_ : 1;

      if (input != '.')
      {
         lastAction_      = input;
         lastActionCount_ = actionCount_;
      }
      
      window_->SetLine("LAST: %u%c COUNT: %u SCROLL: %u", lastActionCount_, lastAction_, actionCount_, ((int) ((screen_.PlaylistWindow().CurrentLine() + 1) * 100)/((int) screen_.PlaylistWindow().TotalNumberOfSongs())) );

      ptrToMember actionFunc = actionTable_[input];
      result = (*this.*actionFunc)(count);
      actionCount_ = 0;

      screen_.Update();
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
