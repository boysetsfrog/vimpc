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

#include "buffers.hpp"
#include "mpdclient.hpp"
#include "settings.hpp"
#include "vimpc.hpp"

#include "buffer/outputs.hpp"
#include "buffer/playlist.hpp"
#include "window/console.hpp"

#include <stdlib.h>
#include <iostream>

using namespace Ui;

Player::Player(Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Main::Settings & settings) :
   screen_      (screen),
   client_      (client),
   clientState_ (clientState),
   playlist_    (Main::Playlist()),
   settings_    (settings)
{

}

Player::~Player()
{

}

void Player::ClearScreen()
{
   screen_.Clear();
}


void Player::Pause()
{
   client_.Pause();
}

void Player::Play(uint32_t position)
{
   screen_.Initialise(Ui::Screen::Playlist);
   client_.Play(position);
}

void Player::Seek(int32_t Offset)
{
   client_.Seek(Offset);
}

void Player::SeekTo(uint32_t Time)
{
   client_.SeekTo(Time);
}


void Player::Quit()
{
   Main::Vimpc::SetRunning(false);
}

void Player::ToggleConsume()
{
   client_.ToggleConsume();
}

void Player::ToggleCrossfade()
{
   client_.ToggleCrossfade();
}

void Player::ToggleRandom()
{
   client_.ToggleRandom();
}

void Player::ToggleRepeat()
{
   client_.ToggleRepeat();
}

void Player::ToggleSingle()
{
   client_.ToggleSingle();
}

void Player::SetRandom(bool random)
{
   client_.SetRandom(random);
}

void Player::SetSingle(bool single)
{
   client_.SetSingle(single);
}

void Player::SetRepeat(bool repeat)
{
   client_.SetRepeat(repeat);
}

void Player::SetConsume(bool consume)
{
   client_.SetConsume(consume);
}

void Player::SetCrossfade(bool crossfade)
{
   client_.SetCrossfade(crossfade);
}

void Player::SetCrossfade(uint32_t crossfade)
{
   client_.SetCrossfade(crossfade);
}

int32_t Player::FindOutput(std::string const & outputName)
{
   // \todo move to output buffeR?
   int32_t output = -1;

   for(unsigned int i = 0; i < Main::Outputs().Size(); ++i)
   {
      if (Algorithm::iequals(Main::Outputs().Get(i)->Name(), outputName) == true)
      {
         output = i;
         break;
      }
   }

   return output;
}

void Player::SetOutput(Item::Collection collection, bool enable)
{
   if (collection == Item::Single)
   {
      int32_t output = screen_.GetSelected(Ui::Screen::Outputs);
      Player::SetOutput(output, enable);
   }
   else if (collection == Item::All)
   {
      for (uint32_t i = 0; i < Main::Outputs().Size(); ++i)
      {
         Player::SetOutput(i, enable);
      }
   }
}

void Player::SetOutput(uint32_t output, bool enable)
{
   if (output < Main::Outputs().Size())
   {
      client_.SetOutput(Main::Outputs().Get(output), enable);
   }
}

void Player::ToggleOutput(Item::Collection collection)
{
   if (collection == Item::Single)
   {
      int32_t output = screen_.GetSelected(Ui::Screen::Outputs);
      Player::ToggleOutput(output);
   }
   else if (collection == Item::All)
   {
      for (uint32_t i = 0; i < Main::Outputs().Size(); ++i)
      {
         Player::ToggleOutput(i);
      }
   }
}

void Player::ToggleOutput(uint32_t output)
{
   if (output < Main::Outputs().Size())
   {
      bool enable = !Main::Outputs().Get(output)->Enabled();
      SetOutput(output, enable);
   }
}

void Player::Shuffle()
{
   client_.Shuffle();
}

void Player::Redraw()
{
   screen_.Redraw();
}

void Player::Stop()
{
   client_.Stop();
}

void Player::Volume(uint32_t volume)
{
   client_.SetVolume(volume);
}


void Player::Rescan()
{
   client_.Rescan("");
}

void Player::Update()
{
   client_.Update("");
}


void Player::SkipSong(Skip skip, uint32_t count)
{
   screen_.Initialise(Ui::Screen::Playlist);

   // If consume or random we have to send a lot of next commands
   // rather than just skipping directly to the right song
   // this is slow and only works in small amounts
   if ((clientState_.Random() == true) || (clientState_.Consume() == true) || (count == 1))
   {
      Mpc::CommandList list(client_, (count != 1));

      for (uint32_t i = 0; i < count; ++i)
      {
         if (skip == Previous)
         {
            client_.Previous();
         }
         else
         {
            client_.Next();
         }
      }
   }
   else
   {
      int64_t directionCount = count;

      if (skip == Previous)
      {
         directionCount *= -1;
      }

      int32_t song = GetCurrentSongPos() + directionCount;

      if (song < 0)
      {
         song = 0;
      }
      else if (song >= static_cast<int32_t>(clientState_.TotalNumberOfSongs()))
      {
         song = clientState_.TotalNumberOfSongs() - 1;
      }

      client_.Play(song);
   }
}

void Player::SkipAlbum(Skip skip, uint32_t count)
{
   screen_.Initialise(Ui::Screen::Playlist);
   SkipSongByInformation(skip, count, &Mpc::Song::Album);
}

void Player::SkipArtist(Skip skip, uint32_t count)
{
   screen_.Initialise(Ui::Screen::Playlist);
   SkipSongByInformation(skip, count, &Mpc::Song::Artist);
}


int32_t Player::GetCurrentSongPos() const
{
   return clientState_.GetCurrentSongPos();
}


void Player::SkipSongByInformation(Skip skip, uint32_t count, Mpc::Song::SongInformationFunction songFunction)
{
   int32_t skipResult = GetCurrentSongPos();

   if (skipResult >= 0)
   {
      for (uint32_t i = 0; i < count; ++i)
      {
         if (skipResult >= 0)
         {
            skipResult = NextSongByInformation(skipResult, skip, songFunction);
         }
      }

      client_.Play(skipResult);
   }
}

uint32_t Player::NextSongByInformation(uint32_t startSong, Skip skip, Mpc::Song::SongInformationFunction songFunction)
{
   if (startSong < playlist_.Size())
   {
      Mpc::Song const * const song         = playlist_.Get(startSong);
      Mpc::Song const *       newSong      = NULL;
      uint32_t                skipCount    = 0;
      int64_t                 direction    = (skip == Previous) ? -1 : 1;

      if ((song != NULL) && (skip == Previous))
      {
         skipCount = First(song, startSong, songFunction);
      }

      if ((song != NULL) && (skipCount == 0))
      {
         do
         {
            ++skipCount;

            if (((skip == Previous) && (startSong >= skipCount)) ||
               ((skip == Next) && (playlist_.Size() > (startSong + skipCount))))
            {
               newSong = playlist_.Get(startSong + (skipCount * direction));
            }
            else
            {
               newSong = NULL;
            }
         }
         while ((newSong != NULL) && ((newSong->*songFunction)().compare((song->*songFunction)()) == 0));

         if (newSong == NULL)
         {
            skipCount = 0;
            newSong   = playlist_.Get(startSong);
         }

         if (skip == Previous)
         {
            skipCount += First(newSong, startSong + (skipCount * direction), songFunction);
         }
      }

      return (startSong + (skipCount * direction));
   }
   else
   {
      return 0;
   }
}


uint32_t Player::First(Mpc::Song const * const song, uint32_t position, Mpc::Song::SongInformationFunction songFunction)
{
   Mpc::Song const * firstSong    = song;
   uint32_t          skipCount    = 0;

   do
   {
      ++skipCount;

      if (position >= skipCount)
      {
         firstSong = playlist_.Get(position - skipCount);
      }
      else
      {
         firstSong = NULL;
      }
   }
   while ((firstSong != NULL) && (firstSong != song) && ((song->*songFunction)().compare((firstSong->*songFunction)()) == 0));

   --skipCount;

   return skipCount;
}


void Player::HandleAutoScroll()
{
   if (screen_.GetActiveWindow() == Screen::Playlist)
   {
      if (settings_.Get(Setting::AutoScroll) == true)
      {
         screen_.ScrollTo(Screen::Current);
      }
   }
}


/* vim: set sw=3 ts=3: */
