/*
   Vimpc
   Copyright (C) 2010 - 2012 Nathan Sweetman

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
#include "settings.hpp"
#include "vimpc.hpp"

#include "mode/mode.hpp"
#include "window/error.hpp"

#include <mpd/tag.h>
#include <mpd/status.h>
#include <sys/time.h>

using namespace Mpc;

#define MPDCOMMAND 


// Helper functions
uint32_t Mpc::SecondsToMinutes(uint32_t duration)
{
   return static_cast<uint32_t>(duration / 60);
}

uint32_t Mpc::RemainingSeconds(uint32_t duration)
{
   return (duration - (SecondsToMinutes(duration) * 60));
}


CommandList::CommandList(Mpc::Client & client, bool condition) :
   condition_(condition),
   client_   (client)
{
   if (condition_)
   {
      client_.ClearCommand();
      client_.StartCommandList();
   }
}

CommandList::~CommandList()
{
   if (condition_)
   {
      client_.SendCommandList();
   }
}


// Mpc::Client Implementation
Client::Client(Main::Vimpc * vimpc, Main::Settings & settings, Ui::Screen & screen) :
   vimpc_                (vimpc),
   settings_             (settings),
   connection_           (NULL),
   hostname_             (""),
   port_                 (0),
   versionMajor_         (-1),
   versionMinor_         (-1),
   versionPatch_         (-1),
   currentSong_          (NULL),
   currentStatus_        (NULL),
   currentSongId_        (-1),
   currentSongURI_       (""),
   currentState_         ("Disconnected"),
   screen_               (screen),
   queueVersion_         (-1),
   forceUpdate_          (true),
   listMode_             (false)
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
   std::string connect_password = "";
   size_t      pos;

   DeleteConnection();

   if (connect_hostname.empty() == true)
   {
      char * const host_env = getenv("MPD_HOST");

      if (host_env != NULL)
      {
         connect_hostname = host_env;

         pos = connect_hostname.find_last_of("@");
         if ( pos != connect_hostname.npos )
         {
            connect_password = connect_hostname.substr(0, pos);
            connect_hostname = connect_hostname.substr(pos + 1);
         }
      }
      else
      {
         connect_hostname = "localhost";
      }
   }

   if (port == 0)
   {
      char * const port_env = getenv("MPD_PORT");

      if (port_env != NULL)
      {
         connect_port = atoi(port_env);
      }
      else
      {
         connect_port = 0;
      }
   }

   // Connecting may take a long time as this is a single threaded application
   // and the mpd connect is a blocking call, so be sure to update the screen
   // first to let the user know that something is happening
   currentState_ = "Connecting";
   vimpc_->CurrentMode().Refresh();

   hostname_ = connect_hostname;
   port_     = connect_port;

   //! \TODO make the connection async
   connection_ = mpd_connection_new(connect_hostname.c_str(), connect_port, 0);

   CheckError();

   if (Connected() == true)
   {
      screen_.Update();
      DisplaySongInformation();
      vimpc_->OnConnected();

      GetVersion();
      UpdateStatus();

      // Must redraw the library first
      screen_.Redraw(Ui::Screen::Library);
      screen_.Redraw(Ui::Screen::Browse);
      screen_.Redraw(Ui::Screen::Lists);
      screen_.Redraw(Ui::Screen::Outputs);
      screen_.Redraw(Ui::Screen::Playlist);

      UpdateStatus();
      CheckForUpdates();

      if (connect_password != "")
      {
         Password(connect_password);
      }
   }
}

void Client::Password(std::string const & password)
{
   if (Connected() == true)
   {
      Command(mpd_send_password(connection_, password.c_str()));
   }
}

std::string Client::Hostname()
{
   return hostname_;
}

uint16_t Client::Port()
{
   return port_;
}

bool Client::Connected() const
{
   return (connection_ != NULL);
}


void Client::Play(uint32_t const playId)
{
   if (Connected() == true)
   {
      Command(mpd_send_play_pos(connection_, playId));
      CheckForUpdates();
   }
}

void Client::Pause()
{
   if (Connected() == true)
   {
      Command(mpd_send_toggle_pause(connection_));
      CheckForUpdates();
   }
}

void Client::Stop()
{
   if (Connected() == true)
   {
      Command(mpd_send_stop(connection_));
      CheckForUpdates();
   }
}

void Client::Next()
{
   if (Connected() == true)
   {
      Command(mpd_send_next(connection_));
      CheckForUpdates();
   }
}

void Client::Previous()
{
   if (Connected() == true)
   {
      Command(mpd_send_previous(connection_));
      CheckForUpdates();
   }
}

void Client::Seek(int32_t Offset)
{
   if (Connected() == true)
   {
      uint32_t const elapsed  = mpd_status_get_elapsed_time(currentStatus_);
      Command(mpd_send_seek_pos(connection_, currentSongId_, elapsed + Offset));
   }
}

void Client::SeekTo(uint32_t Time)
{
   if (Connected() == true)
   {
      Command(mpd_send_seek_pos(connection_, currentSongId_, Time));
   }
}


bool Client::Random()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      return mpd_status_get_random(currentStatus_);
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
      Command(mpd_send_random(connection_, random));
      CheckForUpdates();
   }
}


bool Client::Single()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      return mpd_status_get_single(currentStatus_);
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
      Command(mpd_send_single(connection_, single));
      CheckForUpdates();
   }
}


bool Client::Consume()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      return mpd_status_get_consume(currentStatus_);
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
      Command(mpd_send_consume(connection_, consume));
      CheckForUpdates();
   }
}

bool Client::Repeat()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      return mpd_status_get_repeat(currentStatus_);
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
      Command(mpd_send_repeat(connection_, repeat));
      CheckForUpdates();
   }
}

int32_t Client::Volume()
{
   if ((Connected() == true) && (currentStatus_ != NULL))
   {
      return mpd_status_get_volume(currentStatus_);
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
      Command(mpd_send_set_volume(connection_, volume));
      CheckForUpdates();
   }
}


void Client::Shuffle()
{
   if (Connected() == true)
   {
      Command(mpd_send_shuffle(connection_));
      UpdateStatus();
   }
}

void Client::Move(uint32_t position1, uint32_t position2)
{
   if (Connected() == true)
   {
      Command(mpd_send_move(connection_, position1, position2));
      UpdateStatus(true);
   }
}

void Client::Swap(uint32_t position1, uint32_t position2)
{
   if (Connected() == true)
   {
      Command(mpd_send_swap(connection_, position1, position2));
      UpdateStatus(true);
   }
}


void Client::CreatePlaylist(std::string const & name)
{
   if (Connected() == true)
   {
      Command(mpd_send_save(connection_, name.c_str()));
      Command(mpd_send_playlist_clear(connection_, name.c_str()));
   }
}

void Client::SavePlaylist(std::string const & name)
{
   if (Connected() == true)
   {
      Command(mpd_send_save(connection_, name.c_str()));
      screen_.Redraw(Ui::Screen::Lists);
   }
}

void Client::LoadPlaylist(std::string const & name)
{
   if (Connected() == true)
   {
      Clear();

      Command(mpd_send_load(connection_, name.c_str()));
      UpdateStatus();
   }
}

void Client::RemovePlaylist(std::string const & name)
{
   if (Connected() == true)
   {
      Command(mpd_send_rm(connection_, name.c_str()));
      screen_.Redraw(Ui::Screen::Lists);
   }
}

void Client::AddToPlaylist(std::string const & name, Mpc::Song * song)
{
   if (Connected() == true)
   {
      Command(mpd_send_playlist_add(connection_, name.c_str(), song->URI().c_str()));
   }
}

void Client::EnableOutput(Mpc::Output * output)
{
   if (Connected() == true)
   {
      Command(mpd_send_enable_output(connection_, output->Id()));
      screen_.Redraw(Ui::Screen::Outputs);
   }
}

void Client::DisableOutput(Mpc::Output * output)
{
   if (Connected() == true)
   {
      Command(mpd_send_disable_output(connection_, output->Id()));
      screen_.Redraw(Ui::Screen::Outputs);
   }
}


void Client::Add(Mpc::Song * song)
{
   if ((Connected() == true) && (song != NULL))
   {
      (void) Add(*song);
   }
}

uint32_t Client::Add(Mpc::Song & song)
{
   if (Connected() == true)
   {
      Command(mpd_send_add(connection_, song.URI().c_str()));
      UpdateStatus(true);
   }

   return TotalNumberOfSongs() - 1;
}

uint32_t Client::Add(Mpc::Song & song, uint32_t position)
{
   if (Connected() == true)
   {
      Command(mpd_send_add_id_to(connection_, song.URI().c_str(), position));

      if ((currentSongId_ > -1) && (position <= static_cast<uint32_t>(currentSongId_)))
      {
         ++currentSongId_;
      }

      UpdateStatus(true);
   }

   return TotalNumberOfSongs() - 1;
}

uint32_t Client::AddAllSongs()
{
   if (Connected() == true)
   {
      Command(mpd_send_add(connection_, "/"));
   }

   return TotalNumberOfSongs() - 1;
}

uint32_t Client::Add(std::string const & URI)
{
   if (Connected() == true)
   {
      Command(mpd_send_add(connection_, URI.c_str()));
      UpdateStatus();
   }

   return TotalNumberOfSongs() - 1;
}


void Client::Delete(uint32_t position)
{
   if ((Connected() == true) && (TotalNumberOfSongs() > 0))
   {
      Command(mpd_send_delete(connection_, position));

      if ((currentSongId_ > -1) && (position < static_cast<uint32_t>(currentSongId_)))
      {
         --currentSongId_;
      }

      UpdateStatus(true);
   }
}

void Client::Delete(uint32_t position1, uint32_t position2)
{
   if ((Connected() == true) && (TotalNumberOfSongs() > 0))
   {
      // Only use range if MPD is >= 0.16
      if (versionMinor_ < 16)
      {
         CommandList list(*this);

         for (uint32_t i = 0; i < (position2 - position1); ++i)
         {
            Delete(position1);
         }
      }
      else
      {
         Command(mpd_send_delete_range(connection_, position1, position2));

         if (currentSongId_ > -1)
         {
            uint32_t const songId = static_cast<uint32_t>(currentSongId_);

            if ((position1 < songId) && (position2 < songId))
            {
               currentSongId_ -= position2 - position1;
            }
            else if ((position1 <= songId) && (position2 >= songId))
            {
               currentSongId_ -= (currentSongId_ - position1);
            }
         }
      }

      UpdateStatus(true);
   }
}

void Client::Clear()
{
   if (Connected() == true)
   {
      Command(mpd_send_clear(connection_));
      UpdateStatus(true);
   }
}


void Client::SearchAny(std::string const & search, bool exact)
{
   if (Connected() == true)
   {
      mpd_search_db_songs(connection_, exact);
      mpd_search_add_any_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, search.c_str());
   }
}

void Client::SearchArtist(std::string const & search, bool exact)
{
   if (Connected() == true)
   {
      mpd_search_db_songs(connection_, exact);
      mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_ARTIST, search.c_str());
   }
}

void Client::SearchGenre(std::string const & search, bool exact)
{
   if (Connected() == true)
   {
      mpd_search_db_songs(connection_, exact);
      mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_GENRE, search.c_str());
   }
}



void Client::SearchAlbum(std::string const & search, bool exact)
{
   if (Connected() == true)
   {
      mpd_search_db_songs(connection_, exact);
      mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, search.c_str());
   }
}

void Client::SearchSong(std::string const & search, bool exact)
{
   if (Connected() == true)
   {
      mpd_search_db_songs(connection_, exact);
      mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_TITLE, search.c_str());
   }
}


std::string Client::CurrentState()
{
   if (Connected() == true)
   {
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
   if ((Connected() == true) && (CurrentState() != "Stopped"))
   {
      if ((currentSong_ != NULL) && (currentStatus_ != NULL))
      {
         mpd_status * const status   = currentStatus_;
         uint32_t     const duration = mpd_song_get_duration(currentSong_);
         uint32_t     const elapsed  = mpd_status_get_elapsed_time(status);
         uint32_t     const remain   = duration - elapsed;
         char const * const cArtist  = mpd_song_get_tag(currentSong_, MPD_TAG_ARTIST, 0);
         char const * const cTitle   = mpd_song_get_tag(currentSong_, MPD_TAG_TITLE, 0);
         std::string  const artist   = (cArtist == NULL) ? "Unknown" : cArtist;
         std::string  const title    = (cTitle  == NULL) ? "Unknown" : cTitle;

         screen_.SetStatusLine("[%5u] %s - %s", GetCurrentSong() + 1, artist.c_str(), title.c_str());

         if (settings_.TimeRemaining() == false)
         {
            screen_.MoveSetStatus(screen_.MaxColumns() - 14, "[%2d:%.2d |%2d:%.2d]",
                                  SecondsToMinutes(elapsed),  RemainingSeconds(elapsed),
                                  SecondsToMinutes(duration), RemainingSeconds(duration));
         }
         else
         {
            screen_.MoveSetStatus(screen_.MaxColumns() - 15, "[-%2d:%.2d |%2d:%.2d]",
                                  SecondsToMinutes(remain),  RemainingSeconds(remain),
                                  SecondsToMinutes(duration), RemainingSeconds(duration));
         }
      }
   }
   else
   {
      screen_.SetStatusLine("%s","");
   }
}


void Client::Rescan()
{
   if (Connected() == true)
   {
      Command(mpd_send_rescan(connection_, "/"));
   }
}

void Client::Update()
{
   if (Connected() == true)
   {
      Command(mpd_send_update(connection_, "/"));
   }
}

void Client::IncrementTime(long time)
{
   timeSinceUpdate_ += time;
}

long Client::TimeSinceUpdate()
{
   return timeSinceUpdate_;
}

void Client::CheckForUpdates()
{
   ClearCommand();

   if ((Connected() == true))
   {
      if (listMode_ == false)
      {
         timeSinceUpdate_ = 0;

         if (currentSong_ != NULL)
         {
            mpd_song_free(currentSong_);
            currentSong_ = NULL;
         }

         if( mpd_status_get_state(currentStatus_) != MPD_STATE_STOP)
         {
            currentSong_ = mpd_run_current_song(connection_);
            CheckError();
         }

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

         UpdateStatus();
      }
   }
   else
   {
      currentSongId_ = -1;
      currentSongURI_ = "";
   }
}


void Client::ClearCommand()
{
   if (listMode_ == false)
   {
      Command(false);
   }
}

bool Client::Command(bool InputCommand)
{
   static bool activecommand = false;

   if (Connected() == true)
   {
      if (activecommand == true)
      {
         activecommand = false;
         mpd_response_finish(connection_);
         CheckError();
      }

      if (InputCommand == true)
      {
         activecommand = true;
      }
   }
   else
   {
      activecommand = false;
   }

   return InputCommand;
}

void Client::StartCommandList()
{
   if (Connected() == true)
   {
      listMode_ = true;
      mpd_command_list_begin(connection_, true);
   }
}

void Client::SendCommandList()
{
   if (Connected() == true)
   {
      mpd_command_list_end(connection_);
      mpd_response_finish(connection_);

      CheckError();

      listMode_ = false;
      UpdateStatus(true);
   }
}


unsigned int Client::QueueVersion()
{
   return queueVersion_;
}

void Client::UpdateStatus(bool ExpectUpdate)
{
   ClearCommand();

   if ((Connected() == true) && (listMode_ == false))
   {
      if (currentStatus_ != NULL)
      {
         mpd_status_free(currentStatus_);
         currentStatus_ = NULL;
      }

      currentStatus_ = mpd_run_status(connection_);
      CheckError();

      if (currentStatus_ != NULL)
      {
         unsigned int version  = mpd_status_get_queue_version(currentStatus_);
         unsigned int qVersion = static_cast<uint32_t>(queueVersion_);

         if ((queueVersion_ > -1) &&
             ((version > qVersion + 1) || ((version > qVersion) && (ExpectUpdate == false))))
         {
            ForEachQueuedSongChanges(qVersion, Main::Playlist(), static_cast<void (Mpc::Playlist::*)(uint32_t, Mpc::Song *)>(&Mpc::Playlist::Replace));
            Main::Playlist().Crop(TotalNumberOfSongs());
         }

         queueVersion_ = version;
      }
   }
}


Song * Client::CreateSong(uint32_t id, mpd_song const * const song, bool songInLibrary) const
{
   Song * const newSong = new Song();

   newSong->SetArtist   (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
   newSong->SetAlbum    (mpd_song_get_tag(song, MPD_TAG_ALBUM,  0));
   newSong->SetTitle    (mpd_song_get_tag(song, MPD_TAG_TITLE,  0));
   newSong->SetTrack    (mpd_song_get_tag(song, MPD_TAG_TRACK,  0));
   newSong->SetURI      (mpd_song_get_uri(song));
   newSong->SetDuration (mpd_song_get_duration(song));

   return newSong;
}


void Client::GetVersion()
{
   if (Connected() == true)
   {
      unsigned const * version = mpd_connection_get_server_version(connection_);
      CheckError();

      if (version != NULL)
      {
         versionMajor_ = version[0];
         versionMinor_ = version[1];
         versionPatch_ = version[2];
      }
   }
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

         bool ClearError = mpd_connection_clear_error(connection_);

         if (ClearError == false)
         {
            DeleteConnection();
            currentState_ = "Disconnected";
         }
      }
   }
}

void Client::DeleteConnection()
{
   listMode_ = false;

   versionMajor_ = -1;
   versionMinor_ = -1;
   versionPatch_ = -1;
   queueVersion_ = -1;

   if (connection_ != NULL)
   {
      mpd_connection_free(connection_);
      connection_ = NULL;
   }
}


/* vim: set sw=3 ts=3: */
