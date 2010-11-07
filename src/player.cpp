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
#include "screen.hpp"
#include "song.hpp"

#include <stdlib.h>

using namespace Ui;

Player::Player(Ui::Screen & screen, Mpc::Client & client) :
   screen_(screen),
   client_(client)
{

}

Player::~Player()
{

}

bool Player::Next(uint32_t count)
{
   if (count <= 1)
   {
      Next("");
   }
   else
   {
      client_.Play(GetCurrentSong() + count);
   }

   return true;
}

bool Player::NextArtist(uint32_t count)
{
   uint32_t  const         currentSong  = GetCurrentSong();
   Mpc::Song const * const song         = screen_.PlaylistWindow().GetSong(currentSong);
   Mpc::Song const *       newSong      = NULL;
   uint32_t                skipCount    = 0;

   if (song != NULL)
   {
      do 
      {
         ++skipCount;
         newSong = screen_.PlaylistWindow().GetSong(currentSong + skipCount);
      }
      while ((newSong != NULL) && (newSong->Artist().compare(song->Artist()) == 0));
   }

   client_.Play(currentSong + skipCount);

   return true;
}

bool Player::Previous(uint32_t count)
{
   if (count <= 1)
   {
      Previous("");
   }
   else
   {
      client_.Play(GetCurrentSong() - count);
   }

   return true;
}

bool Player::PreviousArtist(uint32_t count)
{
   uint32_t  const         currentSong  = GetCurrentSong();
   Mpc::Song const * const song         = screen_.PlaylistWindow().GetSong(currentSong);
   Mpc::Song const *       newSong      = NULL;
   uint32_t                skipCount    = 0;

   if (song != NULL)
   {
      do 
      {
         ++skipCount;
         newSong = screen_.PlaylistWindow().GetSong(currentSong - skipCount);
      }
      while ((newSong != NULL) && (newSong->Artist().compare(song->Artist()) == 0));
   }

   client_.Play(currentSong - skipCount);

   return true;
}

bool Player::Connect(std::string const & arguments)
{
   client_.Connect();
   return true;
}

bool Player::ClearScreen(std::string const & arguments)
{
   screen_.Clear();
   return true;
}

bool Player::Quit(std::string const & arguments)
{
   return false;
}

bool Player::Echo(std::string const & arguments)
{
   screen_.ConsoleWindow().OutputLine("%s", arguments.c_str());
   return true;
}

bool Player::Play(std::string const & arguments)
{
   client_.Play(atoi(arguments.c_str()) - 1);
   return true;
}

bool Player::Pause(std::string const & arguments)
{
   client_.Pause();
   return true;
}

bool Player::Next(std::string const & arguments)
{
   client_.Next();
   return true;
}

bool Player::Previous(std::string const & arguments)
{
   client_.Previous();
   return true;
}

bool Player::Stop(std::string const & arguments)
{
   client_.Stop();
   return true;
}

bool Player::Random(std::string const & arguments)
{
   const bool randomOn = (arguments.compare("on") == 0);

   client_.Random(randomOn);
   return true;
}


bool Player::Console(std::string const & arguments)
{
   screen_.SetActiveWindow(Ui::Screen::Console);
   return true;
}

bool Player::Help(std::string const & arguments)
{
   screen_.SetActiveWindow(Ui::Screen::Help);
   return true;
}

bool Player::Playlist(std::string const & arguments)
{
   screen_.SetActiveWindow(Ui::Screen::Playlist);
   return true;
}

bool Player::Library(std::string const & arguments)
{
   screen_.SetActiveWindow(Ui::Screen::Library);
   return true;
}


uint32_t Player::GetCurrentSong() const
{
   return client_.GetCurrentSong();
}
