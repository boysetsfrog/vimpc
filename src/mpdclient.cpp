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
#include "events.hpp"
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
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <future>
#include <signal.h>
#include <sys/types.h>

using namespace Mpc;

#define MPDCOMMAND
//#define _DEBUG_ASSERT_ON_ERROR
//#define _DEBUG_BREAK_ON_ERROR

static std::atomic<bool>                  Running(true);
static std::atomic<int>                   QueueCount(0);
static std::list<std::function<void()> >  Queue;
static std::mutex                         QueueMutex;
static std::condition_variable            Condition;

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
   if (condition_)
   {
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
   fd_                   (-1),

   hostname_             (""),
   port_                 (0),
   versionMajor_         (-1),
   versionMinor_         (-1),
   versionPatch_         (-1),
   timeSinceUpdate_      (0),
   timeSinceSong_        (0),
   retried_              (true),
   ready_                (false),

   volume_               (100),
   mVolume_              (100),
   mute_                 (false),
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
   totalNumberOfSongs_   (0),
   currentSongURI_       (""),
   currentState_         ("Disconnected"),

   screen_               (screen),
   queueVersion_         (-1),
   forceUpdate_          (true),
   listMode_             (false),
   idleMode_             (false)
{
   clientThread_ = std::thread(&Client::ClientQueueExecutor, this, this);

   screen_.RegisterProgressCallback(
      new Main::CallbackObject<Mpc::Client, double>(*this, &Mpc::Client::SeekToPercent));
}

Client::~Client()
{
   Running.store(false);

   clientThread_.join();

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

void Client::QueueCommand(std::function<void()> function)
{
   std::unique_lock<std::mutex> Lock(QueueMutex);
   Queue.push_back(function);
   Condition.notify_all();
   QueueCount.store(Queue.size());
}

void Client::WaitForCompletion()
{
   int Count = QueueCount.load();

   while (Count != 0)
   {
      usleep(20 * 1000);
      Count = QueueCount.load();
   }
}


void Client::Connect(std::string const & hostname, uint16_t port, uint32_t timeout_ms)
{
   QueueCommand([this, hostname, port, timeout_ms] () { this->ConnectImpl(hostname, port, timeout_ms); });
   //ConnectImpl(hostname, port, timeout_ms);
}

void Client::ConnectImpl(std::string const & hostname, uint16_t port, uint32_t timeout_ms)
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
   {
      std::unique_lock<std::recursive_mutex> lock(mutex_);
      currentState_ = "Connecting";

      hostname_   = connect_hostname;
      port_       = connect_port;
      connection_ = NULL;
   }

   //! \TODO make the connection async
   Debug("Client::Connecting to %s:%u - timeout %u", connect_hostname.c_str(), connect_port, connect_timeout);
   struct mpd_connection * connection = mpd_connection_new(connect_hostname.c_str(), connect_port, connect_timeout);

   {
      std::unique_lock<std::recursive_mutex> lock(mutex_);
      connection_ = connection;
   }

   CheckError();

   if (Connected() == true)
   {
      fd_ = mpd_connection_get_fd(connection_);

      Debug("Client::Connected.");

      EventData Data;
      Main::Vimpc::CreateEvent(Event::Connected, Data);

      GetVersion();

      if (connect_password != "")
      {
         Password(connect_password);
      }

      elapsed_ = 0;
      UpdateStatus();
      GetAllMetaInformation();

      if (Connected() == true)
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);
         ready_   = true;
         retried_ = false;
      }
   }
   else
   {
      Error(ErrorNumber::ClientNoConnection, "Failed to connect to server, please ensure it is running and type :connect <server> [port]");
   }
}

void Client::Disconnect()
{
   QueueCommand([this] ()
   {
      if (Connected() == true)
      {
         Debug("Client::Disconnect");
         DeleteConnection();
      }
   });
}

void Client::Reconnect()
{
   QueueCommand([this] ()
   {
      Debug("Client::Reconnect");
      Disconnect();

      std::string hostname = "";
      uint16_t    port     = 0;

      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);
         hostname = hostname_;
         port     = port_;
      }

      Connect(hostname, port);
   });
}

void Client::Password(std::string const & password)
{
   QueueCommand([this, password] ()
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
   });
}

std::string Client::Hostname()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return hostname_;
}

uint16_t Client::Port()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return port_;
}

bool Client::Connected() const
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return (connection_ != NULL);
}

bool Client::Ready() const
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return ready_;
}


void Client::Play(uint32_t const playId)
{
   QueueCommand([this, playId] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Play position %u", playId);

         if (mpd_run_play_pos(connection_, playId) == true)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            currentSongId_ = playId;
            state_ = MPD_STATE_PLAY;
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}


void Client::AddComplete()
{
   QueueCommand([this] ()
   {
      if ((state_ == MPD_STATE_STOP) && (settings_.Get(Setting::PlayOnAdd) == true))
      {
         Debug("Client::Playing start of playlist");
         Play(0);
      }
   });
}

void Client::Pause()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Toggling pause state");

         if (mpd_run_toggle_pause(connection_) == true)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);

            if (state_ == MPD_STATE_PLAY)
            {
               state_ = MPD_STATE_PAUSE;
            }
            else if (state_ == MPD_STATE_PAUSE)
            {
               state_ = MPD_STATE_PLAY;
            }
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Stop()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Stopping playback");

         if (mpd_run_stop(connection_) == true)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            state_ = MPD_STATE_STOP;
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Next()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Next song");
         mpd_send_next(connection_);
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Previous()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Previous song");
         mpd_send_previous(connection_);
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Seek(int32_t Offset)
{
   QueueCommand([this, Offset] ()
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
   });
}

void Client::SeekTo(uint32_t Time)
{
   QueueCommand([this, Time] ()
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
   });
}

void Client::SeekToPercent(double Percent)
{
   QueueCommand([this, Percent] ()
   {
      if (currentSong_)
      {
         Debug("Client::Seek to percent %d%%", static_cast<int32_t>(Percent * 100));

         uint32_t const duration = mpd_song_get_duration(currentSong_);
         SeekTo(static_cast<uint32_t>(Percent * duration));
      }
   });
}


void Client::SetRandom(bool const random)
{
   QueueCommand([this, random] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Set random state %d", static_cast<int32_t>(random));

         if (mpd_run_random(connection_, random) == true)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            random_ = random;

            EventData Data; Data.state = random;
            Main::Vimpc::CreateEvent(Event::Random, Data);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}


bool Client::Single()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return single_;
}

void Client::SetSingle(bool const single)
{
   QueueCommand([this, single] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Set single state %d", static_cast<int32_t>(single));

         if (mpd_run_single(connection_, single) == true)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            single_ = single;
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}


bool Client::Consume()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return consume_;
}

void Client::SetConsume(bool const consume)
{
   QueueCommand([this, consume] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Set consume state %d", static_cast<int32_t>(consume));

         if (mpd_run_consume(connection_, consume) == true)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            consume_ = consume;
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

bool Client::Repeat()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return repeat_;
}

void Client::SetRepeat(bool const repeat)
{
   QueueCommand([this, repeat] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Set repeat state %d", static_cast<int32_t>(repeat));

         if (mpd_run_repeat(connection_, repeat) == true)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            repeat_ = repeat;
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

int32_t Client::Crossfade()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);

   if (crossfade_ == true)
   {
      return crossfadeTime_;
   }

   return 0;
}

void Client::SetCrossfade(bool crossfade)
{
   QueueCommand([this, crossfade] ()
   {
      if (crossfade == true)
      {
         SetCrossfade(crossfadeTime_);
      }
      else
      {
         SetCrossfade(static_cast<uint32_t>(0));
      }
   });
}

void Client::SetCrossfade(uint32_t crossfade)
{
   QueueCommand([this, crossfade] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Set crossfade time %u", crossfade);

         if (mpd_run_crossfade(connection_, crossfade) == true)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            crossfade_ = (crossfade != 0);

            if (crossfade_ == true)
            {
               crossfadeTime_ = crossfade;
            }
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

int32_t Client::Volume()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return volume_;
}

void Client::SetVolume(uint32_t volume)
{
   QueueCommand([this, volume] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Set volume %u", volume);

         if (mpd_run_set_volume(connection_, volume) == true)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            volume_ = volume;
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::SetMute(bool mute)
{
   QueueCommand([this, mute] ()
   {
      bool muteState = false;

      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);
         muteState = mute_;
         mute_ = mute;
      }

      if ((mute == true) && (muteState == false))
      {
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            mVolume_ = volume_;
         }
         SetVolume(0);
      }
      else if ((mute == false) && (muteState == true))
      {
         SetVolume(mVolume_);
      }
   });
}

bool Client::Mute()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return mute_;
}

bool Client::IsUpdating()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return updating_;
}


void Client::Shuffle()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Send shuffle");
         mpd_send_shuffle(connection_);
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Move(uint32_t position1, uint32_t position2)
{
   QueueCommand([this, position1, position2] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Send move %u %u", position1, position2);
         mpd_send_move(connection_, position1, position2);
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Swap(uint32_t position1, uint32_t position2)
{
   QueueCommand([this, position1, position2] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Send swap %u %u", position1, position2);
         mpd_send_swap(connection_, position1, position2);
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}


void Client::CreatePlaylist(std::string const & name)
{
   QueueCommand([this, name] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Send save %s", name.c_str());

         if (mpd_run_save(connection_, name.c_str()) == true)
         {
            if (Main::Lists().Index(Mpc::List(name)) == -1)
            {
               Main::Lists().Add(name);
               Main::Lists().Sort();
            }
         }

         Debug("Client::Send clear playlist %s", name.c_str());
         mpd_run_playlist_clear(connection_, name.c_str());
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::SavePlaylist(std::string const & name)
{
   QueueCommand([this, name] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Send save %s", name.c_str());

         if (mpd_run_save(connection_, name.c_str()) == true)
         {
            if (Main::Lists().Index(Mpc::List(name)) == -1)
            {
               Main::Lists().Add(name);
               Main::Lists().Sort();
            }
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::LoadPlaylist(std::string const & name)
{
   QueueCommand([this, name] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         mpd_run_clear(connection_);
         Debug("Client::Send load %s", name.c_str());
         mpd_run_load(connection_, name.c_str());
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::RemovePlaylist(std::string const & name)
{
   QueueCommand([this, name] ()
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
   });
}

void Client::AddToNamedPlaylist(std::string const & name, Mpc::Song * song)
{
   std::string URI = song->URI();

   QueueCommand([this, name, URI] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Playlist add %s to %s", URI.c_str(), name.c_str());
         mpd_send_playlist_add(connection_, name.c_str(), URI.c_str());
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
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
   uint32_t Id = output->Id();

   QueueCommand([this, Id] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Enable output %d", Id);

         if (mpd_run_enable_output(connection_, Id) == true)
         {
            EventData Data; Data.id = Id;
            Main::Vimpc::CreateEvent(Event::OutputEnabled, Data);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::DisableOutput(Mpc::Output * output)
{
   uint32_t Id = output->Id();

   QueueCommand([this, Id] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Disable output %d", Id);

         if (mpd_run_disable_output(connection_, Id) == true)
         {
            EventData Data; Data.id = Id;
            Main::Vimpc::CreateEvent(Event::OutputDisabled, Data);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}


void Client::Add(Mpc::Song * song)
{
   if (song != NULL)
   {
      (void) Add(*song);
   }
}

void Client::Add(Mpc::Song & song)
{
   std::string URI = song.URI();

   QueueCommand([this, URI] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Add song %s", URI.c_str());
         mpd_send_add(connection_, URI.c_str());

         EventData Data; Data.uri = URI; Data.pos1 = -1;
         Main::Vimpc::CreateEvent(Event::PlaylistAdd, Data);
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Add(Mpc::Song & song, uint32_t position)
{
   std::string URI = song.URI();

   QueueCommand([this, URI, position] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Add song %s at %u", URI.c_str(), position);
         mpd_send_add_id_to(connection_, URI.c_str(), position);

         EventData Data; Data.uri = URI; Data.pos1 = position;
         Main::Vimpc::CreateEvent(Event::PlaylistAdd, Data);

         if ((currentSongId_ > -1) && (position <= static_cast<uint32_t>(currentSongId_)))
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            ++currentSongId_;
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::AddAllSongs()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Add all songs");
         mpd_send_add(connection_, "/");
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Add(std::string const & URI)
{
   QueueCommand([this, URI] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Add uri %s", URI.c_str());
         mpd_send_add(connection_, URI.c_str());
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}


void Client::Delete(uint32_t position)
{
   QueueCommand([this, position] ()
   {
      ClearCommand();

      if ((Connected() == true) && (TotalNumberOfSongs() > 0))
      {
         Debug("Client::Delete position %u", position);
         mpd_send_delete(connection_, position);

         if ((currentSongId_ > -1) && (position < static_cast<uint32_t>(currentSongId_)))
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            --currentSongId_;
         }
      }
      else if (Connected() == false)
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Delete(uint32_t position1, uint32_t position2)
{
   QueueCommand([this, position1, position2] ()
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
                  std::unique_lock<std::recursive_mutex> lock(mutex_);

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
      }

      if (Connected() == false)
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Clear()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Clear");
         mpd_send_clear(connection_);
      }
      else if (Connected() == false)
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}


void Client::SearchAny(std::string const & search, bool exact)
{
   QueueCommand([this, search, exact] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Search any %s - exact %d", search.c_str(), static_cast<int32_t>(exact));
         mpd_search_db_songs(connection_, exact);
         mpd_search_add_any_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, search.c_str());
      }
   });
}

void Client::SearchArtist(std::string const & search, bool exact)
{
   QueueCommand([this, search, exact] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Search artist %s - exact %d", search.c_str(), static_cast<int32_t>(exact));
         mpd_search_db_songs(connection_, exact);
         mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_ARTIST, search.c_str());
      }
   });
}

void Client::SearchGenre(std::string const & search, bool exact)
{
   QueueCommand([this, search, exact] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Search genre %s - exact %d", search.c_str(), static_cast<int32_t>(exact));
         mpd_search_db_songs(connection_, exact);
         mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_GENRE, search.c_str());
      }
   });
}


void Client::SearchAlbum(std::string const & search, bool exact)
{
   QueueCommand([this, search, exact] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Search album %s - exact %d", search.c_str(), static_cast<int32_t>(exact));
         mpd_search_db_songs(connection_, exact);
         mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, search.c_str());
      }
   });
}

void Client::SearchSong(std::string const & search, bool exact)
{
   QueueCommand([this, search, exact] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Search title %s - exact %d", search.c_str(), static_cast<int32_t>(exact));
         mpd_search_db_songs(connection_, exact);
         mpd_search_add_tag_constraint(connection_, MPD_OPERATOR_DEFAULT, MPD_TAG_TITLE, search.c_str());
      }
   });
}


std::string Client::CurrentState()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);

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
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return currentSongURI_;
}

int32_t Client::GetCurrentSongPos()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return currentSongId_;
}

uint32_t Client::TotalNumberOfSongs()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return totalNumberOfSongs_;
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
         screen_.SetProgress(static_cast<double>(elapsed) / duration);
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
   QueueCommand([this, Path] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Rescan %s", (Path != "") ? Path.c_str() : "all");

         if (mpd_run_rescan(connection_, (Path != "") ? Path.c_str() : NULL) == true)
         {
            updating_ = true;
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Update(std::string const & Path)
{
   QueueCommand([this, Path] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Update %s", (Path != "") ? Path.c_str() : "all");

         if (mpd_run_update(connection_, (Path != "") ? Path.c_str() : NULL) == true)
         {
            updating_ = true;
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::IncrementTime(long time)
{
   if (time >= 0)
   {
      std::unique_lock<std::recursive_mutex> lock(mutex_);
      timeSinceUpdate_ += time;
      timeSinceSong_   += time;

      if (state_ == MPD_STATE_PLAY)
      {
         elapsed_ = mpdelapsed_ + (timeSinceUpdate_ / 1000);

         EventData Data;
         Main::Vimpc::CreateEvent(Event::StatusUpdate, Data);
      }

      if ((currentSong_ != NULL) &&
         (elapsed_ >= mpd_song_get_duration(currentSong_)))
      {
         elapsed_ = 0;

         if (timeSinceUpdate_ >= 1000)
         {
            QueueCommand([this, time] ()
            {
               UpdateStatus();
            });
         }
      }
   }
   else
   {
      QueueCommand([this, time] ()
      {
         UpdateStatus();
      });
   }
}

long Client::TimeSinceUpdate()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return timeSinceUpdate_;
}

void Client::IdleMode()
{
   if ((Connected() == true) &&
       (idleMode_ == false) &&
       (ready_ == true) &&
       (settings_.Get(Setting::Polling) == false))
   {
      ClearCommand();

      if ((Connected() == true))
      {
         if (mpd_send_idle(connection_) == true)
         {
            Debug("Client::Enter idle mode");
            idleMode_ = true;
         }
      }
   }
}

void Client::ExitIdleMode()
{
   if ((idleMode_ == true) && (Connected() == true))
   {
      mpd_send_noidle(connection_);

      if (mpd_recv_idle(connection_, false) != 0)
      {
         UpdateStatus();
      }

      Debug("Client::Cancelled idle mode");
      CheckError();
      idleMode_ = false;
   }
}

bool Client::IsIdle()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return idleMode_;
}

bool Client::IsCommandList()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);
   return listMode_;
}

void Client::CheckForEvents()
{
   if ((idleMode_ == true) && (Connected() == true) && (fd_ != -1))
   {
      pollfd fds;

      fds.fd = fd_;
      fds.events = POLLIN;

      if (poll(&fds, 1, 0) > 0)
      {
         idleMode_ = false;

         if (mpd_recv_idle(connection_, false) != 0)
         {
            UpdateStatus();
            Debug("Client::Event occurred");
         }

         Debug("Client::Left idle mode");

         CheckError();
      }
   }
}

void Client::UpdateCurrentSong()
{
   ClearCommand();

   std::unique_lock<std::recursive_mutex> lock(mutex_);

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


void Client::ClientQueueExecutor(Mpc::Client * client)
{
   while (Running.load() == true)
   {
      {
         std::unique_lock<std::mutex> Lock(QueueMutex);

         if ((Queue.empty() == false) ||
             (Condition.wait_for(Lock, std::chrono::milliseconds(100)) != std::cv_status::timeout))
         {
            if (Queue.empty() == false)
            {
               std::function<void()> function = Queue.front();
               Queue.pop_front();
               Lock.unlock();

               ExitIdleMode();
               function();

               Lock.lock();
               QueueCount.store(Queue.size());
               Lock.unlock();
               continue;
            }
         }
      }

      {
         std::unique_lock<std::mutex> Lock(QueueMutex);

         if ((Queue.empty() == true) && (listMode_ == false))
         {
            if (idleMode_ == false)
            {
               Lock.unlock();
               IdleMode();
            }
            else if (idleMode_ == true)
            {
               Lock.unlock();
               CheckForEvents();
            }
         }
      }
   }
}


void Client::ClearCommand()
{
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
   songQueue_.clear();
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
   }

   if (Connected() == true)
   {
      ClearCommand();
      Debug("Client::List queue meta data");
      mpd_send_list_queue_meta(connection_);

      mpd_song * nextSong = mpd_recv_song(connection_);

      for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
      {
         songQueue_.push_back(mpd_song_get_uri(nextSong));
         mpd_song_free(nextSong);
      }
   }

   if (Connected() == true)
   {
      EventData Data;
      Main::Vimpc::CreateEvent(Event::AllMetaDataReady, Data);
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
   QueueCommand([this] ()
   {
      if ((Connected() == true) && (listMode_ == false))
      {
         ClearCommand();

         Debug("Client::Start command list");

         if (mpd_command_list_begin(connection_, false) == true)
         {
            std::unique_lock<std::mutex> Lock(QueueMutex);
            listMode_ = true;
         }
         else
         {
            Debug("Client::Start command list failed");
            CheckError();
         }
      }
   });
}

void Client::SendCommandList()
{
   QueueCommand([this] ()
   {
      if ((Connected() == true) && (listMode_ == true))
      {
         Debug("Client::End command list");

         if (mpd_command_list_end(connection_) == true)
         {
            {
               std::unique_lock<std::mutex> Lock(QueueMutex);
               listMode_ = false;
            }

            EventData Data;
            Main::Vimpc::CreateEvent(Event::CommandListSend, Data);
         }
         else
         {
            Debug("Client::End command list failed");
            CheckError();
         }
      }
   });
}


unsigned int Client::QueueVersion()
{
   return queueVersion_;
}

void Client::UpdateStatus(bool ExpectUpdate)
{
   QueueCommand([this, ExpectUpdate] ()
   {
      ClearCommand();

      if ((Connected() == true) && (listMode_ == false))
      {
         if (currentStatus_ != NULL)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);
            mpd_status_free(currentStatus_);
            currentStatus_ = NULL;
         }

         Debug("Client::Get current status");
         struct mpd_status * status = mpd_run_status(connection_);
         CheckError();

         if (status != NULL)
         {
            std::unique_lock<std::recursive_mutex> lock(mutex_);

            currentStatus_   = status;
            timeSinceUpdate_ = 0;

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
            totalNumberOfSongs_ = mpd_status_get_queue_length(currentStatus_);

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

            lock.unlock();

            EventData Data;
            Main::Vimpc::CreateEvent(Event::StatusUpdate, Data);

            if ((queueVersion_ > -1) &&
               ((version > qVersion + 1) || ((version > qVersion) && (ExpectUpdate == false))))
            {
               ClearCommand();

               if (Connected() == true)
               {
                  songQueueChanges_.clear();
                  Debug("Client::List queue meta data changes");
                  mpd_send_queue_changes_meta(connection_, qVersion);

                  mpd_song * nextSong = mpd_recv_song(connection_);

                  for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
                  {
                     songQueueChanges_.push_back(std::make_pair(mpd_song_get_pos(nextSong), mpd_song_get_uri(nextSong)));
                     mpd_song_free(nextSong);
                  }
               }

               EventData QueueData;
               Main::Vimpc::CreateEvent(Event::QueueUpdate, QueueData);
            }


            if ((wasUpdating == true) && (updating_ == false))
            {
               GetAllMetaInformation();
            }

            queueVersion_ = version;
         }
      }
   });
}

void Client::UpdateCurrentSongPosition()
{
   std::unique_lock<std::recursive_mutex> lock(mutex_);

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
   std::unique_lock<std::recursive_mutex> lock(mutex_);

   ready_        = false;
   listMode_     = false;
   currentState_ = "Disconnected";
   volume_       = -1;
   updating_     = false;
   random_       = false;
   single_       = false;
   consume_      = false;
   repeat_       = false;
   idleMode_     = false;
   crossfade_    = false;

   totalNumberOfSongs_ = 0;

   versionMajor_ = -1;
   versionMinor_ = -1;
   versionPatch_ = -1;
   queueVersion_ = -1;

   // \todo it would be nice to clear the queue here
   // but currently it also removes queued up connect attempts
   // to different hosts, etc, figure this out
   //Queue.clear();

   if (connection_ != NULL)
   {
      mpd_connection_free(connection_);
      connection_ = NULL;
      fd_         = -1;
   }

   lock.unlock();

   EventData Data;
   Main::Vimpc::CreateEvent(Event::Disconnected, Data);

   ENSURE(connection_ == NULL);
}


/* vim: set sw=3 ts=3: */
