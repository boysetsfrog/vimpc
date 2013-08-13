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

#include "buffer/playlist.hpp"
#include "mode/mode.hpp"
#include "window/error.hpp"

#include <mpd/tag.h>
#include <mpd/status.h>
#include <sys/time.h>
#include <poll.h>

using namespace Mpc;

#define MPDCOMMAND
//#define _DEBUG_ASSERT_ON_ERROR
//#define _DEBUG_BREAK_ON_ERROR

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
   list_     (client.IsCommandList()),
   client_   (client)
{
   if ((condition_) && (list_ == false))
   {
      client_.ClearCommand();
      client_.StartCommandList();
   }
}

CommandList::~CommandList()
{
   if ((condition_) && (list_ == false))
   {
      client_.SendCommandList();
   }
}


// Mpc::Client Implementation
Client::Client(Main::Vimpc * vimpc, Main::Settings & settings, Ui::Screen & screen) :
   vimpc_                (vimpc),
   settings_             (settings),
   connection_           (NULL),
   fd_                   (-1),

   hostname_             (""),
   port_                 (0),
   versionMajor_         (-1),
   versionMinor_         (-1),
   versionPatch_         (-1),
   timeSinceUpdate_      (0),
   timeSinceSong_        (0),
   retried_              (true),

   volume_               (100),
   updating_             (false),
   random_               (false),
   repeat_               (false),
   single_               (false),
   consume_              (false),
   crossfade_            (false),
   crossfadeTime_        (0),
   elapsed_              (0),
   state_                (MPD_STATE_STOP),
   mpdstate_             (MPD_STATE_UNKNOWN),

   currentSong_          (NULL),
   currentStatus_        (NULL),
   currentSongId_        (-1),
   currentSongURI_       (""),
   currentState_         ("Disconnected"),

   screen_               (screen),
   queueVersion_         (-1),
   forceUpdate_          (true),
   listMode_             (false),
   idleMode_             (false),
   hadEvents_            (false)
{
   screen_.RegisterProgressCallback(
      new Main::CallbackObject<Mpc::Client, double>(*this, &Mpc::Client::SeekToPercent));
}

Client::~Client()
{
   songs_.clear();

   if (currentStatus_ != NULL)
   {
      mpd_status_free(currentStatus_);
      currentStatus_ = NULL;
   }

   if (currentSong_ != NULL)
   {
      mpd_song_free(currentSong_);
      currentSong_ = NULL;
   }

   DeleteConnection();
}


void Client::Connect(std::string const & hostname, uint16_t port, uint32_t timeout_ms)
{
   std::string connect_hostname = hostname;
   uint16_t    connect_port     = port;
   uint32_t    connect_timeout  = timeout_ms;
   std::string connect_password = "";

   DeleteConnection();

   if (connect_hostname.empty() == true)
   {
      char * const host_env = getenv("MPD_HOST");

      if (host_env != NULL)
      {
         connect_hostname = host_env;

         size_t const pos = connect_hostname.find_last_of("@");

         if (pos != connect_hostname.npos)
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

   if (timeout_ms == 0)
   {
      char * const timeout_env = getenv("MPD_TIMEOUT");

      if (settings_.Get(Setting::Timeout) != "0")
      {
         Debug("Client::Connect timeout " + settings_.Get(Setting::Timeout));
         connect_timeout = atoi(settings_.Get(Setting::Timeout).c_str());
      }
      else if (timeout_env != NULL)
      {
         connect_timeout = atoi(timeout_env);
      }

      connect_timeout *= 1000;
   }

   // Connecting may take a long time as this is a single threaded application
   // and the mpd connect is a blocking call, so be sure to update the screen
   // first to let the user know that something is happening
   currentState_ = "Connecting";
   vimpc_->CurrentMode().Refresh();

   hostname_ = connect_hostname;
   port_     = connect_port;

   //! \TODO make the connection async
   Debug("Client::Connecting to %s:%u - timeout %u", connect_hostname.c_str(), connect_port, connect_timeout);
   connection_ = mpd_connection_new(connect_hostname.c_str(), connect_port, connect_timeout);

   CheckError();

   if (Connected() == true)
   {
      fd_      = mpd_connection_get_fd(connection_);
      retried_ = false;

      screen_.Update();
      DisplaySongInformation();
      vimpc_->OnConnected();

      Debug("Client::Connected.");

      GetVersion();
      UpdateStatus();

      elapsed_ = mpdelapsed_;

      if (connect_password != "")
      {
         Password(connect_password);
      }

      GetAllMetaInformation();
      UpdateStatus();
      IdleMode();
   }
}

void Client::Disconnect()
{
   if (Connected() == true)
   {
      Debug("Client::Disconnect");
      DeleteConnection();
   }
}

void Client::Reconnect()
{
   Debug("Client::Reconnect");
   Disconnect();
   Connect(hostname_, port_);
}

void Client::Password(std::string const & password)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Sending password");
      mpd_send_password(connection_, password.c_str());
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
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
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Play position %u", playId);
      mpd_send_play_pos(connection_, playId);

      currentSongId_ = playId;
      state_ = MPD_STATE_PLAY;
      UpdateStatus();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}


void Client::AddComplete()
{
   if ((state_ == MPD_STATE_STOP) && (settings_.Get(Setting::PlayOnAdd) == true))
   {
      Debug("Client::Playing start of playlist");
      Play(0);
   }
}

void Client::Pause()
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Toggling pause state");
      mpd_send_toggle_pause(connection_);

      if (state_ == MPD_STATE_PLAY)
      {
         state_ = MPD_STATE_PAUSE;
      }
      else if (state_ == MPD_STATE_PAUSE)
      {
         state_ = MPD_STATE_PLAY;
      }
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::Stop()
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Stopping playback");
      mpd_send_stop(connection_);
      state_   = MPD_STATE_STOP;
      UpdateStatus();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::Next()
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Next song");
      mpd_send_next(connection_);
      UpdateStatus();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::Previous()
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Previous song");
      mpd_send_previous(connection_);
      UpdateStatus();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::Seek(int32_t Offset)
{
   ClearCommand();

   if (Connected() == true)
   {
      if (currentSongId_ >= 0)
      {
         Debug("Client::Seek to time %d", elapsed_ + Offset);
         mpd_send_seek_pos(connection_, currentSongId_, elapsed_ + Offset);
      }
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::SeekTo(uint32_t Time)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Seek to time %u", Time);
      mpd_send_seek_pos(connection_, currentSongId_, Time);
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::SeekToPercent(double Percent)
{
   if (currentSong_)
   {
      Debug("Client::Seek to percent %d%%", (int32_t) (Percent * 100));
      uint32_t const duration = mpd_song_get_duration(currentSong_);
      SeekTo((uint32_t) (Percent * duration));
   }
}


bool Client::Random()
{
   return random_;
}

void Client::SetRandom(bool const random)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Set random state %d", (int32_t) random);
      mpd_send_random(connection_, random);
      random_ = random;
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}


bool Client::Single()
{
   return single_;
}

void Client::SetSingle(bool const single)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Set single state %d", (int32_t) single);
      mpd_send_single(connection_, single);
      single_ = single;
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}


bool Client::Consume()
{
   return consume_;
}

void Client::SetConsume(bool const consume)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Set consume state %d", (int32_t) consume);
      mpd_send_consume(connection_, consume);
      consume_ = consume;
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

bool Client::Repeat()
{
   return repeat_;
}

void Client::SetRepeat(bool const repeat)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Set repeat state %d", (int32_t) repeat);
      mpd_send_repeat(connection_, repeat);
      repeat_ = repeat;
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

int32_t Client::Crossfade()
{
   if (crossfade_ == true)
   {
      return crossfadeTime_;
   }

   return 0;
}

void Client::SetCrossfade(bool crossfade)
{
   if (crossfade == true)
   {
      SetCrossfade(crossfadeTime_);
   }
   else
   {
      SetCrossfade(static_cast<uint32_t>(0));
   }
}

void Client::SetCrossfade(uint32_t crossfade)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Set crossfade time %u", crossfade);
      mpd_send_crossfade(connection_, crossfade);
      crossfade_     = (crossfade != 0);

      if (crossfade_ == true)
      {
         crossfadeTime_ = crossfade;
      }
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

int32_t Client::Volume()
{
   return volume_;
}

void Client::SetVolume(uint32_t volume)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Set volume %u", volume);
      mpd_send_set_volume(connection_, volume);
      volume_ = volume;
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

bool Client::IsUpdating()
{
   return updating_;
}


void Client::Shuffle()
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Send shuffle");
      mpd_send_shuffle(connection_);
      UpdateStatus();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::Move(uint32_t position1, uint32_t position2)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Send move %u %u", position1, position2);
      mpd_send_move(connection_, position1, position2);
      UpdateStatus(true);
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::Swap(uint32_t position1, uint32_t position2)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Send swap %u %u", position1, position2);
      mpd_send_swap(connection_, position1, position2);
      UpdateStatus();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}


void Client::CreatePlaylist(std::string const & name)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Send save %s", name.c_str());
      mpd_run_save(connection_, name.c_str());

      Debug("Client::Send clear playlist %s", name.c_str());
      mpd_run_playlist_clear(connection_, name.c_str());
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::SavePlaylist(std::string const & name)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Send save %s", name.c_str());
      mpd_run_save(connection_, name.c_str());
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::LoadPlaylist(std::string const & name)
{
   ClearCommand();

   if (Connected() == true)
   {
      Clear();

      Debug("Client::Send load %s", name.c_str());
      mpd_run_load(connection_, name.c_str());
      UpdateStatus();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::RemovePlaylist(std::string const & name)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Send remove %s", name.c_str());
      mpd_run_rm(connection_, name.c_str());
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::AddToNamedPlaylist(std::string const & name, Mpc::Song * song)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Playlist add %s to %s", song->URI().c_str(), name.c_str());
      mpd_send_playlist_add(connection_, name.c_str(), song->URI().c_str());
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}


void Client::SetOutput(Mpc::Output * output, bool enable)
{
   if (enable == true)
   {
      EnableOutput(output);
   }
   else
   {
      DisableOutput(output);
   }
}

void Client::EnableOutput(Mpc::Output * output)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Enable output %d", output->Id());
      mpd_send_enable_output(connection_, output->Id());
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::DisableOutput(Mpc::Output * output)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Disable output %d", output->Id());
      mpd_send_disable_output(connection_, output->Id());
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}


void Client::Add(Mpc::Song * song)
{
   if ((Connected() == true) && (song != NULL))
   {
      (void) Add(*song);
   }
   else if (Connected() == false)
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

uint32_t Client::Add(Mpc::Song & song)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Add song %s", song.URI().c_str());
      mpd_send_add(connection_, song.URI().c_str());
      UpdateStatus(true);
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }

   return TotalNumberOfSongs() - 1;
}

uint32_t Client::Add(Mpc::Song & song, uint32_t position)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Add song %s at %u", song.URI().c_str(), position);
      mpd_send_add_id_to(connection_, song.URI().c_str(), position);

      if ((currentSongId_ > -1) && (position <= static_cast<uint32_t>(currentSongId_)))
      {
         ++currentSongId_;
      }

      UpdateStatus(true);
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }

   return TotalNumberOfSongs() - 1;
}

uint32_t Client::AddAllSongs()
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Add all songs");
      mpd_send_add(connection_, "/");
      UpdateStatus();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }

   return TotalNumberOfSongs() - 1;
}

uint32_t Client::Add(std::string const & URI)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Add uri %s", URI.c_str());
      mpd_send_add(connection_, URI.c_str());
      UpdateStatus();
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }

   return TotalNumberOfSongs() - 1;
}


void Client::Delete(uint32_t position)
{
   ClearCommand();

   if ((Connected() == true) && (TotalNumberOfSongs() > 0))
   {
      Debug("Client::Delete position %u", position);
      mpd_send_delete(connection_, position);

      if ((currentSongId_ > -1) && (position < static_cast<uint32_t>(currentSongId_)))
      {
         --currentSongId_;
      }

      UpdateStatus(true);
   }
   else if (Connected() == false)
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::Delete(uint32_t position1, uint32_t position2)
{
   if ((Connected() == true) && (TotalNumberOfSongs() > 0))
   {
      // Only use range if MPD is >= 0.16
      if ((versionMajor_ == 0) && (versionMinor_ < 16))
      {
         CommandList list(*this);

         for (uint32_t i = 0; i < (position2 - position1); ++i)
         {
            Delete(position1);
         }
      }
      else
      {
         ClearCommand();

         if (Connected() == true)
         {
            Debug("Client::Delete range %u:%u", position1, position2);
            mpd_send_delete_range(connection_, position1, position2);

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
      }

      UpdateStatus(true);
   }

   if (Connected() == false)
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::Clear()
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Clear");
      mpd_send_clear(connection_);
      UpdateStatus(true);
   }
   else if (Connected() == false)
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}


void Client::SearchAny(std::string const & search, bool exact)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Search any %s - exact %d", search.c_str(), (int32_t) exact);
      mpd_search_db_songs(connection_, exact);
      mpd_search_add_any_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, search.c_str());
   }
}

void Client::SearchArtist(std::string const & search, bool exact)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Search artist %s - exact %d", search.c_str(), (int32_t) exact);
      mpd_search_db_songs(connection_, exact);
      mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_ARTIST, search.c_str());
   }
}

void Client::SearchGenre(std::string const & search, bool exact)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Search genre %s - exact %d", search.c_str(), (int32_t) exact);
      mpd_search_db_songs(connection_, exact);
      mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_GENRE, search.c_str());
   }
}


void Client::SearchAlbum(std::string const & search, bool exact)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Search album %s - exact %d", search.c_str(), (int32_t) exact);
      mpd_search_db_songs(connection_, exact);
      mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, search.c_str());
   }
}

void Client::SearchSong(std::string const & search, bool exact)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Search title %s - exact %d", search.c_str(), (int32_t) exact);
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
         switch (state_)
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

int32_t Client::GetCurrentSongPos()
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
   static char durationStr[128];

   if ((Connected() == true) && (CurrentState() != "Stopped"))
   {
      if ((currentSong_ != NULL) && (currentStatus_ != NULL))
      {
         mpd_status * const status   = currentStatus_;
         uint32_t     const duration = mpd_song_get_duration(currentSong_);
         uint32_t     const elapsed  = elapsed_;
         uint32_t     const remain   = (duration > elapsed) ? duration - elapsed : 0;
         char const * const cArtist  = mpd_song_get_tag(currentSong_, MPD_TAG_ARTIST, 0);
         char const * const cTitle   = mpd_song_get_tag(currentSong_, MPD_TAG_TITLE, 0);
         std::string  const artist   = (cArtist == NULL) ? "Unknown" : cArtist;
         std::string  const title    = (cTitle  == NULL) ? "Unknown" : cTitle;

         screen_.SetStatusLine("[%5u] %s - %s", GetCurrentSongPos() + 1, artist.c_str(), title.c_str());


         if (settings_.Get(Setting::TimeRemaining) == false)
         {
            snprintf(durationStr, 127, "[%d:%.2d/%d:%.2d]",
                     SecondsToMinutes(elapsed),  RemainingSeconds(elapsed),
                     SecondsToMinutes(duration), RemainingSeconds(duration));
         }
         else
         {
            snprintf(durationStr, 127, "[-%d:%.2d/%d:%.2d]",
                     SecondsToMinutes(remain),  RemainingSeconds(remain),
                     SecondsToMinutes(duration), RemainingSeconds(duration));
         }

         screen_.MoveSetStatus(screen_.MaxColumns() - strlen(durationStr), "%s", durationStr);
         screen_.SetProgress((double) elapsed / duration);
      }
      else
      {
         screen_.SetProgress(0);
      }
   }
   else
   {
      screen_.SetStatusLine("%s","");
      screen_.SetProgress(0);
   }
}


void Client::Rescan(std::string const & Path)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Rescan %s", (Path != "") ? Path.c_str() : "all");
      mpd_send_rescan(connection_, (Path != "") ? Path.c_str() : NULL);
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::Update(std::string const & Path)
{
   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Update %s", (Path != "") ? Path.c_str() : "all");
      mpd_send_update(connection_, (Path != "") ? Path.c_str() : NULL);
   }
   else
   {
      ErrorString(ErrorNumber::ClientNoConnection);
   }
}

void Client::IncrementTime(long time)
{
   if (time >= 0)
   {
      timeSinceUpdate_ += time;
      timeSinceSong_   += time;

      if (state_ == MPD_STATE_PLAY)
      {
         elapsed_ = mpdelapsed_ + (timeSinceUpdate_ / 1000);
      }

      if ((currentSong_ != NULL) &&
          (elapsed_ >= mpd_song_get_duration(currentSong_)))
      {
         elapsed_ = 0;

         if (timeSinceUpdate_ >= 1000)
         {
            UpdateStatus();
         }
      }
   }
   else
   {
      UpdateStatus();
   }
}

long Client::TimeSinceUpdate()
{
   return timeSinceUpdate_;
}

void Client::IdleMode()
{
   ClearCommand();

   if ((Connected() == true) && (settings_.Get(Setting::Polling) == false) &&
       (idleMode_ == false))
   {
      if (mpd_send_idle(connection_) == true)
      {
         Debug("Client::Enter idle mode");
         idleMode_ = true;
      }
   }
}

bool Client::IsIdle()
{
   return idleMode_;
}

bool Client::IsCommandList()
{
   return listMode_;
}

bool Client::HadEvents()
{
   if (hadEvents_ == true)
   {
      if (idleMode_ == true)
      {
         mpd_send_noidle(connection_);
         mpd_recv_idle(connection_, false);
         Debug("Client::Cancelled idle mode");

         CheckError();
         idleMode_ = false;
      }

      hadEvents_ = false;
      return true;
   }

   if ((settings_.Get(Setting::Polling) == false) && (idleMode_ == true) && (Connected() == true))
   {
      if (fd_ != -1)
      {
         pollfd fds;

         fds.fd = fd_;
         fds.events = POLLIN;

         if (poll(&fds, 1, 0) > 0)
         {
            idleMode_ = false;
            bool result = (mpd_recv_idle(connection_, false) != 0);
            Debug("Client::Left idle mode");

            if (result == true)
            {
               Debug("Client::Event occurred");
            }

            CheckError();
            return result;
         }
      }
   }

   return false;
}

void Client::UpdateCurrentSong()
{
   ClearCommand();

   if ((Connected() == true))
   {
      if (listMode_ == false)
      {
         if (currentSong_ != NULL)
         {
            mpd_song_free(currentSong_);
            currentSong_ = NULL;
            currentSongId_ = -1;
            currentSongURI_ = "";
         }

         Debug("Client::Send get current song");
         currentSong_ = mpd_run_current_song(connection_);
         timeSinceSong_ = 0;
         CheckError();

         if (currentSong_ != NULL)
         {
            currentSongId_  = mpd_song_get_pos(currentSong_);
            currentSongURI_ = mpd_song_get_uri(currentSong_);

            Debug("Client::Get current song %d:%s", currentSongId_, currentSongURI_.c_str());
         }
      }
   }
   else
   {
      currentSongId_ = -1;
      currentSongURI_ = "";
   }
}

void Client::UpdateDisplay()
{
   // Try and correct display without requesting status from mpd
   UpdateCurrentSongPosition();
}


void Client::ClearCommand()
{
   if ((idleMode_ == true) && (Connected() == true))
   {
      mpd_send_noidle(connection_);
      hadEvents_ = (mpd_recv_idle(connection_, false) != 0);
      Debug("Client::Cancelled idle mode");
      CheckError();
      idleMode_ = false;
   }

   if ((listMode_ == false) && (idleMode_ == false) && (Connected() == true))
   {
      Debug("Client::Finish the response");
      mpd_response_finish(connection_);
      CheckError();
   }
}


void Client::GetAllMetaInformation()
{
   songs_.clear();
   paths_.clear();
   playlists_.clear();

   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Get all meta information");
      mpd_send_list_all_meta(connection_, NULL);

      mpd_entity * nextEntity = mpd_recv_entity(connection_);

      for(; nextEntity != NULL; nextEntity = mpd_recv_entity(connection_))
      {
         if (mpd_entity_get_type(nextEntity) == MPD_ENTITY_TYPE_SONG)
         {
            mpd_song const * const nextSong = mpd_entity_get_song(nextEntity);

            if (nextSong != NULL)
            {
               Song * const newSong = CreateSong(-1, nextSong);
               songs_.push_back(newSong);
            }
         }
         else if (mpd_entity_get_type(nextEntity) == MPD_ENTITY_TYPE_DIRECTORY)
         {
            mpd_directory const * const nextDirectory = mpd_entity_get_directory(nextEntity);

            if (nextDirectory != NULL)
            {
               paths_.push_back(std::string(mpd_directory_get_path(nextDirectory)));
            }
         }
         else if (mpd_entity_get_type(nextEntity) == MPD_ENTITY_TYPE_PLAYLIST)
         {
            mpd_playlist const * const nextPlaylist = mpd_entity_get_playlist(nextEntity);

            if (nextPlaylist != NULL)
            {
               std::string const path = mpd_playlist_get_path(nextPlaylist);
               std::string name = path;

               if (name.find("/") != std::string::npos)
               {
                  name = name.substr(name.find_last_of("/") + 1);
               }

               Mpc::List const list(path, name);
               playlists_.push_back(list);
            }
         }

         mpd_entity_free(nextEntity);
      }

      screen_.InvalidateAll();

      Main::Playlist().Clear();
      Main::PlaylistPasteBuffer().Clear();
      Main::Library().Clear();

      ForEachLibrarySong(Main::Library(), &Mpc::Library::Add);
      ForEachQueuedSong(Main::Playlist(), static_cast<void (Mpc::Playlist::*)(Mpc::Song *)>(&Mpc::Playlist::Add));
   }

#if !LIBMPDCLIENT_CHECK_VERSION(2,5,0)
   GetAllMetaFromRoot();
#endif
}

void Client::GetAllMetaFromRoot()
{
   // This is a hack to get playlists when using older libmpdclients, it should
   // not be used unless absolutely necessary
   playlistsOld_.clear();

   ClearCommand();

   if (Connected() == true)
   {
      Debug("Client::Get all root meta");
      mpd_send_list_meta(connection_, "/");

      mpd_entity * nextEntity = mpd_recv_entity(connection_);

      for(; nextEntity != NULL; nextEntity = mpd_recv_entity(connection_))
      {
         if (mpd_entity_get_type(nextEntity) == MPD_ENTITY_TYPE_PLAYLIST)
         {
            mpd_playlist const * const nextPlaylist = mpd_entity_get_playlist(nextEntity);

            if (nextPlaylist != NULL)
            {
               std::string const path = mpd_playlist_get_path(nextPlaylist);
               std::string name = path;

               if (name.find("/") != std::string::npos)
               {
                  name = name.substr(name.find_last_of("/") + 1);
               }

               Mpc::List const list(path, name);
               playlistsOld_.push_back(list);
            }
         }

         mpd_entity_free(nextEntity);
      }

      screen_.Invalidate(Ui::Screen::Lists);
   }
}


void Client::StartCommandList()
{
   if (Connected() == true)
   {
      Debug("Client::Start command list");
      mpd_command_list_begin(connection_, false);

      if (CheckError() == false)
      {
         listMode_ = true;
      }
   }
}

void Client::SendCommandList()
{
   if ((Connected() == true) && (listMode_ == true))
   {
      Debug("Client::End command list");
      listMode_ = false;
      mpd_command_list_end(connection_);
      CheckError();

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

      Debug("Client::Get current status");
      timeSinceUpdate_ = 0;
      currentStatus_   = mpd_run_status(connection_);
      CheckError();

      if (currentStatus_ != NULL)
      {
         unsigned int version     = mpd_status_get_queue_version(currentStatus_);
         unsigned int qVersion    = static_cast<uint32_t>(queueVersion_);
         bool const   wasUpdating = updating_;

         volume_   = mpd_status_get_volume(currentStatus_);
         updating_ = (mpd_status_get_update_id(currentStatus_) >= 1);
         random_   = mpd_status_get_random(currentStatus_);
         repeat_   = mpd_status_get_repeat(currentStatus_);
         single_   = mpd_status_get_single(currentStatus_);
         consume_  = mpd_status_get_consume(currentStatus_);
         crossfade_ = (mpd_status_get_crossfade(currentStatus_) > 0);

         if (crossfade_ == true)
         {
            crossfadeTime_ = mpd_status_get_crossfade(currentStatus_);
         }

         // Check if we need to update the current song
         if ((mpdstate_ != mpd_status_get_state(currentStatus_)) ||
             ((mpdstate_ != MPD_STATE_STOP) && (currentSong_ == NULL)) ||
             ((mpdstate_ == MPD_STATE_STOP) && (currentSong_ != NULL)) ||
             ((currentSong_ != NULL) &&
              ((elapsed_ >= mpd_song_get_duration(currentSong_) - 3) ||
               (mpd_status_get_elapsed_time(currentStatus_) < mpdelapsed_) ||
               (mpd_status_get_elapsed_time(currentStatus_) <= 3))))
         {
            UpdateCurrentSong();
         }

         mpdstate_   = mpd_status_get_state(currentStatus_);
         mpdelapsed_ = mpd_status_get_elapsed_time(currentStatus_);
         state_      = mpdstate_;

         if (mpdstate_ == MPD_STATE_STOP)
         {
            currentSongId_  = -1;
            currentSongURI_ = "";
         }

         if (mpdstate_ != MPD_STATE_PLAY)
         {
            elapsed_ = mpdelapsed_;
         }

         if ((queueVersion_ > -1) &&
             ((version > qVersion + 1) || ((version > qVersion) && (ExpectUpdate == false))))
         {
            Main::PlaylistTmp().Clear();

            for (int i = 0; i < Main::PlaylistPasteBuffer().Size(); ++i)
            {
               Main::PlaylistTmp().Add(Main::PlaylistPasteBuffer().Get(i));
            }

            ForEachQueuedSongChanges(qVersion, Main::Playlist(), static_cast<void (Mpc::Playlist::*)(uint32_t, Mpc::Song *)>(&Mpc::Playlist::Replace));
            Main::Playlist().Crop(TotalNumberOfSongs());

            // Ensure that the queue related updates don't break our paste buffer
            Main::PlaylistPasteBuffer().Clear();

            for (int i = 0; i < Main::PlaylistTmp().Size(); ++i)
            {
               Main::PlaylistPasteBuffer().Add(Main::PlaylistTmp().Get(i));
            }

            Main::PlaylistTmp().Clear();
         }

         if ((wasUpdating == true) && (updating_ == false))
         {
            GetAllMetaInformation();
         }

         queueVersion_ = version;
      }
   }
}

void Client::UpdateCurrentSongPosition()
{
   if ((currentSong_ != NULL) && (currentSongId_ >= 0) &&
       (currentSongId_ < static_cast<int32_t>(Main::Playlist().Size())) &&
       (*Main::Playlist().Get(currentSongId_) != *currentSong_))
   {
      currentSongId_ = -1;

      for (uint32_t i = 0; i < screen_.MaxRows(); ++i)
      {
         int32_t id = i + screen_.ActiveWindow().FirstLine();

         if ((id < static_cast<int32_t>(Main::Playlist().Size())) &&
             (*Main::Playlist().Get(id) == *currentSong_))
         {
            currentSongId_ = id;
            break;
         }
      }
   }
}


Song * Client::CreateSong(uint32_t id, mpd_song const * const song, bool songInLibrary) const
{
   Song * const newSong = new Song();

   char const * artist = NULL;

   if (settings_.Get(Setting::AlbumArtist) == true)
   {
      artist = mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0);
   }

   newSong->SetArtist   ((artist == NULL) ? mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) : artist);
   newSong->SetAlbum    (mpd_song_get_tag(song, MPD_TAG_ALBUM,  0));
   newSong->SetTitle    (mpd_song_get_tag(song, MPD_TAG_TITLE,  0));
   newSong->SetTrack    (mpd_song_get_tag(song, MPD_TAG_TRACK,  0));
   newSong->SetURI      (mpd_song_get_uri(song));
   newSong->SetGenre    (mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
   newSong->SetDate     (mpd_song_get_tag(song, MPD_TAG_DATE, 0));
   newSong->SetDuration (mpd_song_get_duration(song));

   return newSong;
}


void Client::GetVersion()
{
   if (Connected() == true)
   {
      Debug("Client::Sending version request");
      unsigned const * version = mpd_connection_get_server_version(connection_);
      CheckError();

      if (version != NULL)
      {
         versionMajor_ = version[0];
         versionMinor_ = version[1];
         versionPatch_ = version[2];

         Debug("libmpdclient: %d.%d.%d", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);
         Debug("MPD Server  : %d.%d.%d", versionMajor_, versionMinor_, versionPatch_);
      }
   }
}

bool Client::CheckError()
{
   if (connection_ != NULL)
   {
      if (mpd_connection_get_error(connection_) != MPD_ERROR_SUCCESS)
      {
         char error[255];
         snprintf(error, 255, "MPD Error: %s",  mpd_connection_get_error_message(connection_));
         Error(ErrorNumber::ClientError, error);

         Debug("Client::%s", error);

#ifdef _DEBUG_ASSERT_ON_ERROR
         ASSERT(false);
#elif defined(_DEBUG_BREAK_ON_ERROR)
         BREAKPOINT
#endif

         bool ClearError = mpd_connection_clear_error(connection_);

         if (ClearError == false)
         {
            Debug("Client::Unable to clear error");
            DeleteConnection();

            if ((settings_.Get(Setting::Reconnect) == true) && (retried_ == false))
            {
               retried_ = true;
               Connect(hostname_, port_);
            }
         }
         else
         {
            UpdateStatus();
         }

         return true;
      }
   }

   return false;
}

void Client::DeleteConnection()
{
   listMode_     = false;
   currentState_ = "Disconnected";
   volume_       = -1;
   updating_     = false;
   random_       = false;
   single_       = false;
   consume_      = false;
   repeat_       = false;
   idleMode_     = false;

   versionMajor_ = -1;
   versionMinor_ = -1;
   versionPatch_ = -1;
   queueVersion_ = -1;

   if (connection_ != NULL)
   {
      mpd_connection_free(connection_);
      connection_ = NULL;
      fd_         = -1;
   }

   ENSURE(connection_ == NULL);
}


/* vim: set sw=3 ts=3: */
