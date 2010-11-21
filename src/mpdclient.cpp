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

#include "assert.hpp"
#include "error.hpp"
#include "console.hpp"
#include "playlist.hpp"
#include "screen.hpp"

#include <mpd/tag.h>
#include <mpd/status.h>

using namespace Mpc;

Client::Client(Ui::Screen const & screen) :
   connection_(NULL),
   screen_    (screen)
{
}

Client::~Client()
{
   // \todo make a function for this
   if (connection_ != NULL)
   {
      mpd_connection_free(connection_);
      connection_ = NULL;
   }
}

void Client::Connect(std::string const & hostname)
{
   // \todo needs to take parameters and such properly
   // rather than just using the defaults only
   if (connection_ != NULL)
   {
      mpd_connection_free(connection_);
      connection_ = NULL;
   }

   if (hostname == "")
   {
      connection_ = mpd_connection_new("localhost", 0, 0);
   }
   else
   {
      connection_ = mpd_connection_new(hostname.c_str(), 0, 0);
   }

   screen_.PlaylistWindow().Redraw();
   DisplaySongInformation();

   CheckError();
}

void Client::Play(uint32_t const playId)
{
   if (Connected() == true)
   {
      mpd_run_play_id(connection_, playId);
      DisplaySongInformation();

      CheckError();
   }
}

void Client::Pause()
{
   if (Connected() == true)
   {
      mpd_run_toggle_pause(connection_);
      DisplaySongInformation();

      CheckError();
   }
}

void Client::Next()
{
   if (Connected() == true)
   {
      mpd_run_next(connection_);
      DisplaySongInformation();

      CheckError();
   }
}

void Client::Previous()
{
   if (Connected() == true)
   {
      mpd_run_previous(connection_);
      DisplaySongInformation();

      CheckError();
   }
}

void Client::Stop()
{
   if (Connected() == true)
   {
      mpd_run_stop(connection_);
      DisplaySongInformation();

      CheckError();
   }
}

void Client::Random(bool const randomOn)
{
   if (Connected() == true)
   {
      mpd_run_random(connection_, randomOn);

      CheckError();
   }
}


bool Client::Connected() const
{
   return (connection_ != NULL);
}


int32_t Client::GetCurrentSongId() const
{
   int32_t songId = - 1;

   if (Connected() == true)
   {
      mpd_song * currentSong = mpd_run_current_song(connection_);

      if (currentSong != NULL)
      {
         songId = mpd_song_get_id(currentSong);
         mpd_song_free(currentSong);
      }
   }

   return songId;
}

int32_t Client::TotalNumberOfSongs()
{
   int32_t songTotal = -1;

   if (Connected() == true)
   {
      mpd_status * const status = mpd_run_status(connection_);

      if (status != NULL)
      {
         songTotal = mpd_status_get_queue_length(status);
         mpd_status_free(status);
      }
   }

   CheckError();

   return songTotal;
}

void Client::DisplaySongInformation()
{
   if (Connected() == true)
   {
      mpd_song * currentSong = mpd_run_current_song(connection_);

      if (currentSong != NULL)
      {
         mpd_status * const status = mpd_run_status(connection_);
         uint32_t const songId     = mpd_song_get_id(currentSong);
         uint32_t const duration   = mpd_song_get_duration(currentSong);

         if (status != NULL)
         {
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
   }
}

void Client::CheckError()
{
   if (mpd_connection_get_error(connection_) != MPD_ERROR_SUCCESS)
   {
      // \todo fix and make critical error
      char error[255];
      snprintf(error, 255, "Client Error: %s",  mpd_connection_get_error_message(connection_));
      Error(2, error);

      if (connection_ != NULL)
      {
         mpd_connection_free(connection_);
         connection_ = NULL;
      }
   }
}
