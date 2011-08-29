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
#include "screen.hpp"
#include "window/error.hpp"

#include <mpd/tag.h>
#include <mpd/status.h>

using namespace Mpc;


// Local helper functions
namespace Mpc
{
   uint32_t SecondsToMinutes(uint32_t duration);
   uint32_t RemainingSeconds(uint32_t duration);
}

uint32_t Mpc::SecondsToMinutes(uint32_t duration)
{
   return static_cast<uint32_t>(duration / 60);
}

uint32_t Mpc::RemainingSeconds(uint32_t duration)
{
   return (duration - (SecondsToMinutes(duration) * 60));
}


// Mpc::Client Implementation
Client::Client(Ui::Screen const & screen) :
   connection_           (NULL),
   screen_               (screen)
{
}

Client::~Client()
{
   DeleteConnection();
}


void Client::Connect(std::string const & hostname, uint16_t port)
{
   DeleteConnection();

   connection_ = mpd_connection_new(hostname.c_str(), port, 0);

   // Must redraw the library first
   screen_.Redraw(Ui::Screen::Library);
   screen_.Redraw(Ui::Screen::Browse);
   screen_.Redraw(Ui::Screen::Playlist);
   CheckError();
}

void Client::Play(uint32_t const playId)
{
   if (Connected() == true)
   {
      uint32_t id = playId;
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

void Client::Stop()
{
   if (Connected() == true)
   {
      mpd_run_stop(connection_);
      CheckError();
   }
}

void Client::Next()
{
   if (Connected() == true)
   {
      mpd_run_next(connection_);
      CheckError();
   }
}

void Client::Previous()
{
   if (Connected() == true)
   {
      mpd_run_previous(connection_);
      CheckError();
   }
}


bool Client::Random()
{
   if (Connected() == true)
   {
      mpd_status * status = mpd_run_status(connection_);
      CheckError();

      bool const random = mpd_status_get_random(status);
      mpd_status_free(status);
      return random;
   }
   else
   {
      return false;
   }
}

void Client::SetRandom(bool const random)
{
   if (Connected() == true)
   {
      mpd_run_random(connection_, random);
      CheckError();
   }
}


bool Client::Single()
{
   if (Connected() == true)
   {
      mpd_status * status = mpd_run_status(connection_);
      CheckError();

      bool const single = mpd_status_get_single(status);
      mpd_status_free(status);
      return single;
   }
   else
   {
      return false;
   }
}

void Client::SetSingle(bool const single)
{
   if (Connected() == true)
   {
      mpd_run_single(connection_, single);
      CheckError();
   }
}


bool Client::Consume()
{
   if (Connected() == true)
   {
      mpd_status * status = mpd_run_status(connection_);
      CheckError();

      bool const consume = mpd_status_get_consume(status);
      mpd_status_free(status);
      return consume;
   }
   else
   {
      return false;
   }
}

void Client::SetConsume(bool const consume)
{
   if (Connected() == true)
   {
      mpd_run_consume(connection_, consume);
      CheckError();
   }
}

bool Client::Repeat()
{
   if (Connected() == true)
   {
      mpd_status * status = mpd_run_status(connection_);
      CheckError();

      bool const repeat = mpd_status_get_repeat(status);
      mpd_status_free(status);
      return repeat;
   }
   else
   {
      return false;
   }
}

void Client::SetRepeat(bool const repeat)
{
   if (Connected() == true)
   {
      mpd_run_repeat(connection_, repeat);
      CheckError();
   }
}

int32_t Client::Volume()
{
   if (Connected() == true)
   {
      mpd_status * status = mpd_run_status(connection_);
      CheckError();

      int32_t const volume = mpd_status_get_volume(status);
      mpd_status_free(status);
      return volume;
   }
   else
   {
      return -1;
   }
}

void Client::SetVolume(uint32_t volume)
{
   if (Connected() == true)
   {
      (void) mpd_run_set_volume(connection_, volume);
      CheckError();
   }
}

uint32_t Client::Add(Mpc::Song & song)
{
   if (Connected() == true)
   {
      CheckForUpdates();

      mpd_run_add(connection_, song.URI().c_str());
      queueVersion_ = QueueVersion();

      CheckError();
   }

   return TotalNumberOfSongs() - 1;
}

uint32_t Client::Add(Mpc::Song & song, uint32_t position)
{
   if (Connected() == true)
   {
      CheckForUpdates();

      mpd_run_add_id_to(connection_, song.URI().c_str(), position);
      queueVersion_ = QueueVersion();

      CheckError();
   }

   return TotalNumberOfSongs() - 1;
}

void Client::Delete(uint32_t position)
{
   if ((Connected() == true) && (TotalNumberOfSongs() > 0))
   {
      CheckForUpdates();

      mpd_run_delete(connection_, position);
      queueVersion_ = QueueVersion();
      CheckError();
   }
}

void Client::Clear()
{
   if (Connected() == true)
   {
      mpd_run_clear(connection_);
      queueVersion_ = QueueVersion();
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

void Client::CheckForUpdates()
{
   if (Connected() == true)
   {
      if (queueVersion_ != QueueVersion())
      {
         queueVersion_ = QueueVersion();
         screen_.Redraw(Ui::Screen::Playlist);
      }

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

std::string Client::GetCurrentSongURI()
{
   std::string song;

   if (Connected() == true)
   {
      mpd_song * currentSong = mpd_run_current_song(connection_);

      CheckError();

      if (currentSong != NULL)
      {
         song = mpd_song_get_uri(currentSong);
         mpd_song_free(currentSong);
      }
   }

   return song;
}

//! \todo rename to GetCurrentSongPos
int32_t Client::GetCurrentSong()
{
   static int32_t song = -1;

   if (Connected() == true)
   {
      mpd_song * currentSong = mpd_run_current_song(connection_);

      CheckError();

      if (currentSong != NULL)
      {
         song = mpd_song_get_pos(currentSong);
         mpd_song_free(currentSong);
      }
   }

   return song;
}

uint32_t Client::TotalNumberOfSongs()
{
   uint32_t songTotal = 0;

   if (Connected() == true)
   {
      mpd_status * const status = mpd_run_status(connection_);

      CheckError();

      if (status != NULL)
      {
         songTotal = mpd_status_get_queue_length(status);
         mpd_status_free(status);
      }
   }

   CheckError();

   return songTotal;
}

bool Client::SongIsInQueue(Mpc::Song const & song) const
{
   return (song.Reference() != 0);
}


void Client::DisplaySongInformation()
{
   // \todo should cache this information

   if ((Connected() == true) && (CurrentState() != "Stopped"))
   {
      mpd_song * currentSong = mpd_run_current_song(connection_);

      CheckError();

      if (currentSong != NULL)
      {
         mpd_status * const status   = mpd_run_status(connection_);
         uint32_t     const duration = mpd_song_get_duration(currentSong);
         uint32_t     const elapsed  = mpd_status_get_elapsed_time(status);
         char const * const cArtist  = mpd_song_get_tag(currentSong, MPD_TAG_ARTIST, 0);
         char const * const cTitle   = mpd_song_get_tag(currentSong, MPD_TAG_TITLE, 0);
         std::string  const artist   = (cArtist == NULL) ? "Unknown" : cArtist;
         std::string  const title    = (cTitle  == NULL) ? "Unknown" : cTitle;

         //! \todo turn into a single setstatus and use a blank filler rather than a move
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

unsigned int Client::QueueVersion()
{
   if (Connected() == true)
   {
      mpd_status * status = mpd_run_status(connection_);

      if (status != NULL)
      {
         unsigned int Version = mpd_status_get_queue_version(status);
         mpd_status_free(status);
         return Version;
      }
   }
}


Song * Client::CreateSong(uint32_t id, mpd_song const * const song) const
{
   Song * const newSong = new Song();

   newSong->SetArtist  (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
   newSong->SetAlbum   (mpd_song_get_tag(song, MPD_TAG_ALBUM,  0));
   newSong->SetTitle   (mpd_song_get_tag(song, MPD_TAG_TITLE,  0));
   newSong->SetTrack   (mpd_song_get_tag(song, MPD_TAG_TRACK,  0));
   newSong->SetURI     (mpd_song_get_uri(song));
   newSong->SetDuration(mpd_song_get_duration(song));

   return newSong;
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
            screen_.Redraw(Ui::Screen::Library);
            screen_.Redraw(Ui::Screen::Playlist);
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


