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
#include "library.hpp"
#include "playlist.hpp"
#include "screen.hpp"

#include <iomanip>
#include <sstream>

#include <sys/time.h>
#include <unistd.h>
#include <mpd/tag.h>
#include <mpd/status.h>

using namespace Mpc;

Client::Client(Ui::Screen const & screen) :
   connection_           (NULL),
   currentSongHasChanged_(true),
   screen_               (screen)
{
}

Client::~Client()
{
   DeleteConnection();
}


bool Client::GetRandom()
{
   mpd_status * status = mpd_run_status(connection_);

   return mpd_status_get_random(status);
}


void Client::Connect(std::string const & hostname, uint16_t port)
{
   // \todo needs to take parameters and such properly
   // rather than just using the defaults only
   DeleteConnection();

   connection_ = mpd_connection_new(hostname.c_str(), port, 0);

   screen_.LibraryWindow().Redraw();
   screen_.PlaylistWindow().Redraw();
   CheckError();
}

void Client::Play(uint32_t const playId)
{
   if (Connected() == true)
   {
      uint32_t id = playId;
      currentSongHasChanged_ = true;

      mpd_run_play_pos(connection_, id);
      CheckError();
   }
}

void Client::Pause()
{
   if (Connected() == true)
   {
      mpd_run_toggle_pause(connection_);
      CheckError();
   }
}

void Client::Next()
{
   if (Connected() == true)
   {
      currentSongHasChanged_ = true;

      mpd_run_next(connection_);
      CheckError();
   }
}

void Client::Previous()
{
   if (Connected() == true)
   {
      currentSongHasChanged_ = true;

      mpd_run_previous(connection_);
      CheckError();
   }
}

void Client::Stop()
{
   if (Connected() == true)
   {
      mpd_run_stop(connection_);
      CheckError();
   }
}

void Client::Random(bool const random)
{
   if (Connected() == true)
   {
      mpd_run_random(connection_, random);
      CheckError();
   }
}

void Client::Single(bool const single)
{
   if (Connected() == true)
   {
      mpd_run_single(connection_, single);
      CheckError();
   }
}


uint32_t Client::Add(Mpc::Song & song)
{
   int32_t position = screen_.PlaylistWindow().Playlist().size();

   if (Connected() == true)
   {
      mpd_run_add(connection_, song.URI().c_str());

      song.IncrementReference();

      screen_.PlaylistWindow().AddSong(&song, position);

      CheckError();
   }

   return TotalNumberOfSongs() - 1;
}

void Client::Delete(uint32_t position)
{
   if ((Connected() == true) && (TotalNumberOfSongs() > 0))
   {
      mpd_run_delete(connection_, position);
   }
}

void Client::Clear()
{
   if (Connected() == true)
   {
      mpd_run_clear(connection_);
      CheckError();
   }
}


void Client::Rescan()
{ 
   if (Connected() == true)
   {
      mpd_run_rescan(connection_, "/");
      CheckError();
   }
}

void Client::Update()
{
   if (Connected() == true)
   {
      mpd_run_update(connection_, "/");

      CheckError();
   }
}


std::string Client::CurrentState()
{
   std::string currentState("Disconnected");

   if (Connected() == true)
   {
      mpd_status * const status = mpd_run_status(connection_);

      if (status != NULL)
      {
         mpd_state state = mpd_status_get_state(status);

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

            default:
               break;
         }

         mpd_status_free(status);
      }
   }

   return currentState;
}


bool Client::Connected() const
{
   return (connection_ != NULL);
}

std::string Client::GetCurrentSongURI() const
{
   std::string song;

   mpd_song * currentSong = mpd_run_current_song(connection_);

   if (currentSong != NULL)
   {
      song = mpd_song_get_uri(currentSong);
   }

   return song;
}

//! \todo rename to GetCurrentSongPos
int32_t Client::GetCurrentSong() const
{
   static int32_t song = -1;

   mpd_song * currentSong = mpd_run_current_song(connection_);

   if (currentSong != NULL)
   {
      song = mpd_song_get_pos(currentSong);
   }

   return song;
}

uint32_t Client::TotalNumberOfSongs()
{
   uint32_t songTotal = 0;

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

bool Client::SongIsInQueue(Mpc::Song & song) const
{
   return (song.Reference() != 0);
}


void Client::DisplaySongInformation()
{
   // \todo should cache this information

   if ((Connected() == true) && (CurrentState() != "Stopped"))
   {
      mpd_song * currentSong = mpd_run_current_song(connection_);

      if (currentSong != NULL)
      {
         mpd_status * const status   = mpd_run_status(connection_);
         uint32_t     const duration = mpd_song_get_duration(currentSong);
         uint32_t     const elapsed  = mpd_status_get_elapsed_time(status);
         std::string  const artist   = mpd_song_get_tag(currentSong, MPD_TAG_ARTIST, 0);
         std::string  const title    = mpd_song_get_tag(currentSong, MPD_TAG_TITLE, 0);

         screen_.SetStatusLine("[%5u] %s - %s", GetCurrentSong() + 1, artist.c_str(), title.c_str());
         screen_.MoveSetStatus(screen_.MaxColumns() - 14, "[%2d:%.2d |%2d:%.2d]", 
                               SecondsToMinutes(elapsed),  RemainingSeconds(elapsed),
                               SecondsToMinutes(duration), RemainingSeconds(duration));

         mpd_status_free(status);
         mpd_song_free(currentSong);
      }
   }
   else
   {
      screen_.SetStatusLine("");
   }
}


Song * Client::CreateSong(UNUSED uint32_t id, mpd_song const * const song) const
{
   Song * const newSong = new Song();

   newSong->SetArtist  (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
   newSong->SetAlbum   (mpd_song_get_tag(song, MPD_TAG_ALBUM,  0));
   newSong->SetTitle   (mpd_song_get_tag(song, MPD_TAG_TITLE,  0));
   newSong->SetURI     (mpd_song_get_uri(song)); 
   newSong->SetDuration(mpd_song_get_duration(song));

   return newSong;
}


uint32_t Client::SecondsToMinutes(uint32_t duration) const
{
   return static_cast<uint32_t>(duration / 60);
}

uint32_t Client::RemainingSeconds(uint32_t duration) const
{
   return (duration - (SecondsToMinutes(duration) * 60));
}
   


void Client::CheckError()
{
   if (connection_ != NULL)
   {
      if (mpd_connection_get_error(connection_) != MPD_ERROR_SUCCESS)
      {
         char error[255];
         snprintf(error, 255, "Client Error: %s",  mpd_connection_get_error_message(connection_));
         Error(ErrorNumber::ClientError, error);

         if (connection_ != NULL)
         {
            screen_.PlaylistWindow().Redraw();
            screen_.LibraryWindow().Redraw();
         }

         DeleteConnection();
      }
   }
}

void Client::DeleteConnection()
{
   if (connection_ != NULL)
   {
      mpd_connection_free(connection_);
      connection_ = NULL;
   }
}


