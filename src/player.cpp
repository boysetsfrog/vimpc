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

   player.cpp - common music playing related commands 
   */

#include "player.hpp"

#include "console.hpp"
#include "mpdclient.hpp"
#include "playlist.hpp"
#include "settings.hpp"
#include "song.hpp"

#include <stdlib.h>

using namespace Ui;

Player::Player(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings) :
   screen_  (screen),
   client_  (client),
   settings_(settings)
{

}

Player::~Player()
{

}

bool Player::ClearScreen()
{
   screen_.Clear();
   return true;
}

bool Player::Connect(std::string const & host, uint32_t port)
{
   // \todo handle port properly
   client_.Connect(host);
   //client_.Connect(host, port);

   return true;
}

bool Player::Echo(std::string const & echo)
{
   screen_.ConsoleWindow().OutputLine("%s", echo.c_str());
   return true;
}



bool Player::Pause()
{
   client_.Pause();
   return true;
}

bool Player::Play(uint32_t id)
{
   client_.Play(id - 1);
   return true;
}

bool Player::Quit()
{
   return false;
}

bool Player::Random(bool random)
{
   client_.Random(random);
   return true;
}

bool Player::Redraw()
{
   screen_.Redraw();
   return true;
}

bool Player::SetActiveWindow(Ui::Screen::MainWindow window)
{
   screen_.SetActiveWindow(window);
   return true;
}

bool Player::Stop()
{
   client_.Stop();
   return true;
}


bool Player::SkipSong(Skip skip, uint32_t count)
{
   int64_t directionCount = count;

   if (skip == Previous)
   {
      directionCount *= -1;
   }

   client_.Play(GetCurrentSong() + directionCount);

   HandleAutoScroll();

   return true;
}

bool Player::SkipArtist(Skip skip, uint32_t count)
{
   uint32_t  const         currentSong  = GetCurrentSong();
   Mpc::Song const * const song         = screen_.PlaylistWindow().GetSong(currentSong);
   Mpc::Song const *       newSong      = NULL;
   uint32_t                skipCount    = 0;
   int64_t                 direction    = (skip == Previous) ? -1 : 1;

   if (song != NULL)
   {
      do 
      {
         ++skipCount;
         newSong = screen_.PlaylistWindow().GetSong(currentSong + (skipCount * direction));
      }
      while ((newSong != NULL) && (newSong->Artist().compare(song->Artist()) == 0));
   }

   client_.Play(currentSong + (skipCount * direction));

   HandleAutoScroll();

   return true;
}

uint32_t Player::GetCurrentSong() const
{
   return client_.GetCurrentSong();
}

void Player::HandleAutoScroll()
{
   if (settings_.AutoScroll() == true)
   {
      screen_.ScrollTo(Screen::Current);
   }
}






