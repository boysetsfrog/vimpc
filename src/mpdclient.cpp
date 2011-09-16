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
#include "vimpc.hpp"

#include "mode/mode.hpp"
#include "window/error.hpp"

#include <mpd/tag.h>
#include <mpd/status.h>
#include <sys/time.h>

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
Client::Client(Main::Vimpc * vimpc, Ui::Screen & screen) :
   vimpc_                (vimpc),
   connection_           (NULL),
   currentSong_          (NULL),
   currentStatus_        (NULL),
   currentSongId_        (-1),
   currentSongURI_       (""),
   currentState_         ("Disconnected"),
   screen_               (screen),
   queueVersion_         (0),
   forceUpdate_          (true)
{
}

Client::~Client()
{
   DeleteConnection();
}


void Client::Connect(std::string const & hostname, uint16_t port)
{
   std::string connect_hostname = hostname;
   uint16_t    connect_port     = port;

   DeleteConnection();

   if (connect_hostname.empty() == true)
   {
      char * const host_env = getenv("MPD_HOST");
      char * const port_env = getenv("MPD_PORT");

      if (host_env != NULL)
      {
         connect_hostname = host_env;
      }
      else
      {
         connect_hostname = "localhost";
      }

      if (port_env != NULL)
      {
         connect_port = atoi(port_env);
      }
   }

   // Connecting may take a long time as this is a single threaded application
   // and the mpd connect is a blocking call, so be sure to update the screen
   // first to let the user know that something is happening
   currentState_ = "Connecting";
   screen_.Update();
   vimpc_->CurrentMode().Refresh();

   //! \TODO I may need to end up using threads in here, or lower the default timeout at least
   connection_ = mpd_connection_new(connect_hostname.c_str(), connect_port, 0);

   // Throw away any keys that were pressed in anger, while we were waiting to connect
   screen_.ClearInput();

   CheckError();

   if (Connected() == true)
   {
      // Must redraw the library first
      screen_.Redraw(Ui::Screen::Library);
      screen_.Redraw(Ui::Screen::Browse);
      screen_.Redraw(Ui::Screen::Lists);

      // This will redraw the playlist window
      CheckForUpdates();
   }
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
      forceUpdate_ = true;
      mpd_run_toggle_pause(connection_);
      CheckError();
   }
}

void Client::Stop()
{
   if (Connected() == true)
   {
      forceUpdate_ = true;
      mpd_run_stop(connection_);
      CheckError();
   }
}

void Client::Next()
{
   if (Connected() == true)
   {
      forceUpdate_ = true;
      mpd_run_next(connection_);
      CheckError();
   }
}

void Client::Previous()
{
   if (Connected() == true)
   {
      forceUpdate_ = true;
      mpd_run_previous(connection_);
      CheckError();
   }
}


bool Client::Random()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      CheckForUpdates();

      bool const random = mpd_status_get_random(currentStatus_);
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
      forceUpdate_ = true;
      mpd_run_random(connection_, random);
      CheckError();
   }
}


bool Client::Single()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      CheckForUpdates();

      bool const single = mpd_status_get_single(currentStatus_);
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
      forceUpdate_ = true;
      mpd_run_single(connection_, single);
      CheckError();
   }
}


bool Client::Consume()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      CheckForUpdates();

      bool const consume = mpd_status_get_consume(currentStatus_);
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
      forceUpdate_ = true;
      mpd_run_consume(connection_, consume);
      CheckError();
   }
}

bool Client::Repeat()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      CheckForUpdates();

      bool const repeat = mpd_status_get_repeat(currentStatus_);
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
      forceUpdate_ = true;
      mpd_run_repeat(connection_, repeat);
      CheckError();
   }
}

int32_t Client::Volume()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      CheckForUpdates();

      int32_t const volume = mpd_status_get_volume(currentStatus_);
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
      forceUpdate_ = true;
      (void) mpd_run_set_volume(connection_, volume);
      CheckError();
   }
}

void Client::SavePlaylist(std::string const & name)
{
   if (Connected() == true)
   {
      (void) mpd_run_save(connection_, name.c_str());
      CheckError();

      screen_.Redraw(Ui::Screen::Lists);
   }
}

void Client::LoadPlaylist(std::string const & name)
{
   if (Connected() == true)
   {
      Clear();

      (void) mpd_run_load(connection_, name.c_str());
      CheckError();

      screen_.Redraw(Ui::Screen::Playlist);
   }
}

uint32_t Client::Add(Mpc::Song & song)
{
   if (Connected() == true)
   {
      CheckForUpdates();

      mpd_run_add(connection_, song.URI().c_str());
      CheckError();

      queueVersion_ = QueueVersion();
   }

   return TotalNumberOfSongs() - 1;
}

uint32_t Client::Add(Mpc::Song & song, uint32_t position)
{
   if (Connected() == true)
   {
      CheckForUpdates();

      mpd_run_add_id_to(connection_, song.URI().c_str(), position);
      CheckError();

      queueVersion_ = QueueVersion();
   }

   return TotalNumberOfSongs() - 1;
}

uint32_t Client::AddAllSongs()
{
   if (Connected() == true)
   {
      mpd_run_add(connection_, "/");
      CheckError();
      CheckForUpdates();
   }

   return TotalNumberOfSongs() - 1;
}

void Client::Delete(uint32_t position)
{
   if ((Connected() == true) && (TotalNumberOfSongs() > 0))
   {
      CheckForUpdates();

      mpd_run_delete(connection_, position);
      CheckError();

      queueVersion_ = QueueVersion();
   }
}

void Client::Clear()
{
   if (Connected() == true)
   {
      mpd_run_clear(connection_);
      CheckError();

      queueVersion_ = QueueVersion();
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
   static bool   updated = false;
   static struct timeval start;
   static struct timeval end;

   gettimeofday(&end, NULL);

   if (Connected() == true)
   {
      if (queueVersion_ != QueueVersion())
      {
         queueVersion_ = QueueVersion();
         screen_.Redraw(Ui::Screen::Playlist);
      }

      long const seconds  = end.tv_sec  - start.tv_sec;
      long const useconds = end.tv_usec - start.tv_usec;
      long const mtime    = (seconds * 1000 + (useconds/1000.0)) + 0.5;

      if ((updated == false) || (mtime > 250) || (forceUpdate_ == true))
      {
         forceUpdate_ = false;
         updated      = true;

         if (currentSong_ != NULL)
         {
            mpd_song_free(currentSong_);
            currentSong_ = NULL;
         }

         if (currentStatus_ != NULL)
         {
            mpd_status_free(currentStatus_);
            currentStatus_ = NULL;
         }

         currentSong_ = mpd_run_current_song(connection_);
         CheckError();

         if (Connected() == true)
         {
            currentStatus_ = mpd_run_status(connection_);
            CheckError();

            if (currentSong_ != NULL)
            {
                currentSongId_  = mpd_song_get_pos(currentSong_);
                currentSongURI_ = mpd_song_get_uri(currentSong_);
            }
            else
            {
                currentSongId_  = -1;
                currentSongURI_ = "";
            }
         }
         else
         {
             currentSongId_ = -1;
             currentSongURI_ = "";
         }

         gettimeofday(&start, NULL);
      }
   }
   else
   {
      currentSongId_ = -1;
      currentSongURI_ = "";
   }
}


std::string Client::CurrentState()
{
   if (Connected() == true)
   {
      CheckForUpdates();

      if (currentStatus_ != NULL)
      {
         mpd_state state = mpd_status_get_state(currentStatus_);

         switch (state)
         {
            case MPD_STATE_UNKNOWN:
               currentState_ = "Unknown";
               break;
            case MPD_STATE_STOP:
               currentState_ = "Stopped";
               break;
            case MPD_STATE_PLAY:
               currentState_ = "Playing";
               break;
            case MPD_STATE_PAUSE:
               currentState_ = "Paused";
               break;

            default:
               break;
         }
      }
   }

   return currentState_;
}


bool Client::Connected() const
{
   return (connection_ != NULL);
}

std::string Client::GetCurrentSongURI()
{
   return currentSongURI_;
}

//! \todo rename to GetCurrentSongPos
int32_t Client::GetCurrentSong()
{
   return currentSongId_;
}

uint32_t Client::TotalNumberOfSongs()
{
   uint32_t songTotal = 0;

   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      CheckForUpdates();

      songTotal = mpd_status_get_queue_length(currentStatus_);
   }

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
      CheckForUpdates();

      if ((currentSong_ != NULL) && (currentStatus_ != NULL))
      {
         mpd_status * const status   = currentStatus_;
         uint32_t     const duration = mpd_song_get_duration(currentSong_);
         uint32_t     const elapsed  = mpd_status_get_elapsed_time(status);
         char const * const cArtist  = mpd_song_get_tag(currentSong_, MPD_TAG_ARTIST, 0);
         char const * const cTitle   = mpd_song_get_tag(currentSong_, MPD_TAG_TITLE, 0);
         std::string  const artist   = (cArtist == NULL) ? "Unknown" : cArtist;
         std::string  const title    = (cTitle  == NULL) ? "Unknown" : cTitle;

         //! \todo turn into a single setstatus and use a blank filler rather than a move
         screen_.SetStatusLine("[%5u] %s - %s", GetCurrentSong() + 1, artist.c_str(), title.c_str());
         screen_.MoveSetStatus(screen_.MaxColumns() - 14, "[%2d:%.2d |%2d:%.2d]",
                               SecondsToMinutes(elapsed),  RemainingSeconds(elapsed),
                               SecondsToMinutes(duration), RemainingSeconds(duration));
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

   return 0;
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

         //! \TODO figure out why every error currently requires me to kill the connection
         // this is really really wrong
         DeleteConnection();

         currentState_ = "Disconnected";
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


