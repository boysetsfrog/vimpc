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

   mpdclient.cpp - provides interaction with the music player daemon 
   */

#include "mpdclient.hpp"

#include "console.hpp"
#include "dbc.hpp"
#include "playlist.hpp"
#include "screen.hpp"
#include "song.hpp"

#include <mpd/tag.h>
#include <mpd/status.h>

using namespace Mpc;

Client::Client(Ui::Screen & screen) :
   started_   (false),
   screen_    (screen),
   connection_(NULL)
{
}

Client::~Client()
{
   if (connection_ != NULL)
   {
      mpd_connection_free(connection_);
      connection_ = NULL;
   }
}

void Client::Start()
{
   REQUIRE(started_ == false);

   if (started_ == false)
   {
      started_ = true;

      // \todo check that we are connected?
      mpd_send_list_queue_meta(connection_);

      mpd_song * nextSong  = mpd_recv_song(connection_);

      for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
      {
         Song * newSong = new Song(mpd_song_get_id(nextSong) + 1);

         newSong->SetArtist(mpd_song_get_tag(nextSong, MPD_TAG_ARTIST, 0));
         newSong->SetTitle (mpd_song_get_tag(nextSong, MPD_TAG_TITLE,  0));

         screen_.PlaylistWindow().AddSong(newSong);

         mpd_song_free(nextSong);
      }

      DisplaySongInformation();
   }

   ENSURE(started_ == true);
}


void Client::Connect()
{
   // \todo needs to take parameters and such properly
   // rather than just using the defaults only
   connection_ = mpd_connection_new("127.0.0.1", 0, 0);

   CheckError();
}

void Client::Play(uint32_t const playId)
{
   mpd_run_play_id(connection_, playId);
   DisplaySongInformation();

   CheckError();
}

void Client::Pause()
{
   mpd_run_toggle_pause(connection_);
   DisplaySongInformation();

   CheckError();
}

void Client::Next()
{
   mpd_run_next(connection_);
   DisplaySongInformation();

   CheckError();
}

void Client::Previous()
{
   mpd_run_previous(connection_);
   DisplaySongInformation();

   CheckError();
}

void Client::Stop()
{
   mpd_run_stop(connection_);
   DisplaySongInformation();

   CheckError();
}

void Client::Random(bool const randomOn)
{
   mpd_run_random(connection_, randomOn);

   CheckError();
}

int32_t Client::GetCurrentSong() const
{
   int32_t songId = - 1;
   mpd_song * currentSong = mpd_run_current_song(connection_);

   if (currentSong != NULL)
   {
      songId = mpd_song_get_id(currentSong);
      mpd_song_free(currentSong);
   }

   return songId;
}

int32_t Client::TotalNumberOfSongs() const
{
   return mpd_status_get_queue_length(mpd_run_status(connection_));
}

void Client::DisplaySongInformation()
{
   mpd_song * currentSong = mpd_run_current_song(connection_);

   if (currentSong != NULL)
   {
      mpd_status * status  = mpd_run_status(connection_);
      uint32_t songId      = mpd_song_get_id(currentSong);
      uint32_t duration    = mpd_song_get_duration(currentSong);

      mpd_state state = mpd_status_get_state(status);

      std::string currentState;

      switch (state)
      {
         case MPD_STATE_UNKNOWN:
            currentState = "Unknown";
            break;
         case MPD_STATE_STOP:
            currentState = "Stopped";
            break;
         case MPD_STATE_PLAY:
            currentState = "Playing";
            break;
         case MPD_STATE_PAUSE:
            currentState = "Paused";
            break;
      }

      uint32_t const minutes = static_cast<uint32_t>(duration / 60);
      uint32_t const seconds = (duration - (minutes * 60));

      screen_.SetStatusLine("[%s...][%u/%u] %s - %s [%s | %s] [%d:%.2d]", currentState.c_str(), songId + 1, mpd_status_get_queue_length(status),
            mpd_song_get_tag(currentSong, MPD_TAG_ARTIST, 0), mpd_song_get_tag(currentSong, MPD_TAG_TITLE,0), mpd_song_get_tag(currentSong, MPD_TAG_TRACK,0),
            mpd_song_get_tag(currentSong, MPD_TAG_ALBUM,0), minutes, seconds);

      mpd_song_free(currentSong);
      mpd_status_free(status);
   }
}

void Client::CheckError()
{
   if (mpd_connection_get_error(connection_) != MPD_ERROR_SUCCESS)
   {
      screen_.ConsoleWindow().OutputLine("Client Error: %s", mpd_connection_get_error_message(connection_));
   }
}
