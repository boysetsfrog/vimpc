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

#include "attributes.hpp"
#include "console.hpp"
#include "mpdclient.hpp"
#include "playlist.hpp"
#include "settings.hpp"

#include <stdlib.h>
#include <iostream>

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

bool Player::Connect(std::string const & host, UNUSED uint32_t port)
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


bool Player::Rescan()
{
   client_.Rescan();
   return true;
}

bool Player::Update()
{
   client_.Update();
   return true;
}


bool Player::SkipSong(Skip skip, uint32_t count)
{
   int64_t directionCount = count;

   if (skip == Previous)
   {
      directionCount *= -1;
   }

   int32_t song = GetCurrentSong() + directionCount;

   if ((GetCurrentSong() + directionCount) < 0)
   {
      song = 0;
   }
   else if ((GetCurrentSong() + directionCount) >= client_.TotalNumberOfSongs())
   {
      song = client_.TotalNumberOfSongs() - 1;
   }

   client_.Play(song);

   HandleAutoScroll();

   return true;
}

bool Player::SkipAlbum(Skip skip, uint32_t count)
{
   SkipSongByInformation(skip, count, &Mpc::Song::Album);
   return true;
}

bool Player::SkipArtist(Skip skip, uint32_t count)
{
   SkipSongByInformation(skip, count, &Mpc::Song::Artist);
   return true;
}


uint32_t Player::GetCurrentSong() const
{
   return client_.GetCurrentSongId();
}


bool Player::SkipSongByInformation(Skip skip, uint32_t count, Mpc::Song::SongInformationFunction songFunction)
{
   uint32_t skipResult = GetCurrentSong();

   for (uint32_t i = 0; i < count; ++i)
   {
      skipResult = NextSongByInformation(skipResult, skip, songFunction);
   }

   client_.Play(skipResult);

   HandleAutoScroll();

   return true;
}

uint32_t Player::NextSongByInformation(uint32_t startSong, Skip skip, Mpc::Song::SongInformationFunction songFunction)
{
   Mpc::Song const * const song         = screen_.PlaylistWindow().GetSong(startSong);
   Mpc::Song const *       newSong      = NULL;
   uint32_t                skipCount    = 0;
   int64_t                 direction    = (skip == Previous) ? -1 : 1;

   if ((song != NULL) && (skip == Previous))
   {
      skipCount = First(song, songFunction);
   }

   if ((song != NULL) && (skipCount == 0))
   {
      do 
      {
         ++skipCount;
         newSong = screen_.PlaylistWindow().GetSong(startSong + (skipCount * direction));
      }
      while ((newSong != NULL) && ((newSong->*songFunction)().compare((song->*songFunction)()) == 0));

      if (newSong == NULL)
      {
         skipCount = 0;
         newSong   = screen_.PlaylistWindow().GetSong(startSong);
      }

      if (skip == Previous)
      {
         skipCount += First(newSong, songFunction);
      }
   }

   return (startSong + (skipCount * direction));
}


uint32_t Player::First(Mpc::Song const * const song, Mpc::Song::SongInformationFunction songFunction)
{
   Mpc::Song const * firstSong    = song;
   uint32_t          skipCount    = 0;

   do
   {
      ++skipCount;
      firstSong = screen_.PlaylistWindow().GetSong(song->Id() - skipCount - 1);
   }
   while ((firstSong != NULL) && (firstSong != song) && ((song->*songFunction)().compare((firstSong->*songFunction)()) == 0));

   --skipCount;

   return skipCount;
}


void Player::HandleAutoScroll()
{
   if (settings_.AutoScroll() == true)
   {
      screen_.ScrollTo(Screen::Current);
   }

}






