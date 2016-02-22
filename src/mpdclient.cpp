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
#include "buffer/list.hpp"
#include "mode/mode.hpp"
#include "window/error.hpp"

#include <mpd/client.h>
#include <sys/time.h>
#include <poll.h>
#include <unistd.h>

#include <list>
#include <signal.h>
#include <sys/types.h>

using namespace Mpc;

#define MPDCOMMAND
//#define _DEBUG_ASSERT_ON_ERROR
//#define _DEBUG_BREAK_ON_ERROR

static std::list<FUNCTION<void()> >       Queue;
static Mutex                              QueueMutex;
static Atomic(bool)                       Running(true);
static ConditionVariable                  Condition;


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
   Debug("Commandlist object construct");

   if (condition_)
   {
      client_.StartCommandList();
   }
}

CommandList::~CommandList()
{
   Debug("Commandlist object destruct");

   if (condition_)
   {
      client_.SendCommandList();
   }
}


// Mpc::Client Implementation
Client::Client(Main::Vimpc * vimpc, Main::Settings & settings, Mpc::Lists & lists, Ui::Screen & screen) :
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
   state_                (MPD_STATE_UNKNOWN),
   mpdstate_             (MPD_STATE_UNKNOWN),

   currentSong_          (NULL),
   currentStatus_        (NULL),
   currentSongId_        (-1),
   totalNumberOfSongs_   (0),
   currentSongURI_       (""),
   currentState_         ("Disconnected"),

   lists_                (&lists),
   loadedList_           (""),

   screen_               (screen),
   queueVersion_         (-1),
   oldVersion_           (-1),
   forceUpdate_          (true),
   listMode_             (false),
   idleMode_             (false),
   queueUpdate_          (false),
   autoscroll_           (false),
   clientThread_         (Thread(&Client::ClientQueueExecutor, this, this))
{
   screen_.RegisterProgressCallback([this] (double Value) { SeekToPercent(Value); });

   Main::Vimpc::EventHandler(Event::PlaylistContentsForRemove, [this] (EventData const & Data)
   {
      Mpc::CommandList list(*this);

      for (auto uri : Data.uris)
      {
         Mpc::Song * song = Main::Library().Song(uri);

         int const PlaylistIndex = Main::Playlist().Index(song);

         if (PlaylistIndex >= 0)
         {
            Delete(PlaylistIndex);
         }
      }
   });
}

Client::~Client()
{
   Running = false;

   clientThread_.join();

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

void Client::QueueCommand(FUNCTION<void()> const & function)
{
   UniqueLock<Mutex> Lock(QueueMutex);
   Queue.push_back(function);
   Condition.notify_all();
}

void Client::WaitForCompletion()
{
   QueueMutex.lock();
   int Count = Queue.size();
   QueueMutex.unlock();

   while (Count != 0)
   {
      usleep(20 * 1000);
      UniqueLock<Mutex> Lock(QueueMutex);
      Count = Queue.size();
   }
}


void Client::Connect(std::string const & hostname, uint16_t port, uint32_t timeout_ms)
{
   QueueCommand([this, hostname, port, timeout_ms] () { ConnectImpl(hostname, port, timeout_ms); });
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
   currentState_ = "Connecting";
   StateEvent();

   hostname_   = connect_hostname;
   port_       = connect_port;
   connection_ = NULL;

   EventData HostData;
   HostData.hostname = hostname_;
   HostData.port     = port_;
   Main::Vimpc::CreateEvent(Event::ChangeHost, HostData);

   //! \TODO make the connection async
   Debug("Client::Connecting to %s:%u - timeout %u", connect_hostname.c_str(), connect_port, connect_timeout);
   connection_ = mpd_connection_new(connect_hostname.c_str(), connect_port, connect_timeout);

   CheckError();

   if (Connected() == true)
   {
      fd_ = mpd_connection_get_fd(connection_);

      Debug("Client::Connected.");

      EventData Data;
      Main::Vimpc::CreateEvent(Event::Connected, Data);
      Main::Vimpc::CreateEvent(Event::Repaint,   Data);

      GetVersion();

      if (connect_password != "")
      {
         Password(connect_password);
      }

      elapsed_ = 0;

      if (IsPasswordRequired() == false)
      {
         Initialise();
      }
      else
      {
         StateEvent();
         EventData Data;
         Main::Vimpc::CreateEvent(Event::RequirePassword, Data);
      }
   }
   else
   {
      Error(ErrorNumber::ClientNoConnection, "Failed to connect to server, please ensure it is running and type :connect <server> [port]");
   }
}

void Client::Initialise()
{
   UpdateStatus();
   StateEvent();

   GetAllMetaInformation();
   UpdateCurrentSong();

   if (Connected() == true)
   {
      ready_   = true;
      retried_ = false;
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
      Connect(hostname_, port_);
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

bool Client::Connected() const
{
   return (connection_ != NULL);
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
            currentSongId_ = playId;
            EventData IdData; IdData.id = currentSongId_;
            Main::Vimpc::CreateEvent(Event::CurrentSongId, IdData);

            autoscroll_ = false;
            Main::Vimpc::CreateEvent(Event::Autoscroll,    IdData);

            state_ = MPD_STATE_PLAY;
            elapsed_ = 0;
            timeSinceUpdate_ = 0;
            StateEvent();
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
            if (state_ == MPD_STATE_PLAY)
            {
               state_ = MPD_STATE_PAUSE;
            }
            else if (state_ == MPD_STATE_PAUSE)
            {
               state_ = MPD_STATE_PLAY;
            }

            timeSinceUpdate_ = 0;
            elapsed_ = mpdelapsed_;
            StateEvent();
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
            state_ = MPD_STATE_STOP;
            StateEvent();
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

         autoscroll_ = true;
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

         autoscroll_ = true;
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
         if (currentSongId_ >= 0)
         {
            Debug("Client::Seek to time %u", Time);
            mpd_send_seek_pos(connection_, currentSongId_, Time);
         }
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
            SetStateAndEvent(Event::Random, random_, random);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
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
            SetStateAndEvent(Event::Single, single_, single);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
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
            SetStateAndEvent(Event::Consume, consume_, consume);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
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
            SetStateAndEvent(Event::Repeat, repeat_, repeat);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
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
            crossfade_ = (crossfade != 0);

            if (crossfade_ == true)
            {
               crossfadeTime_ = crossfade;
               EventData Data; Data.value = crossfade;
               Main::Vimpc::CreateEvent(Event::CrossfadeTime, Data);
            }

            EventData Data; Data.state = crossfade_;
            Main::Vimpc::CreateEvent(Event::Crossfade, Data);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
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
            volume_ = volume;

            EventData Data; Data.value = volume;
            Main::Vimpc::CreateEvent(Event::Volume, Data);
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
      if ((mute == true) && (mute_ == false))
      {
         mVolume_ = volume_;
         SetVolume(0);
      }
      else if ((mute == false) && (mute_ == true))
      {
         SetVolume(mVolume_);
      }

      mute_ = mute;
      EventData Data; Data.state = mute_;
      Main::Vimpc::CreateEvent(Event::Mute, Data);
   });
}

void Client::DeltaVolume(int32_t Delta)
{
   QueueCommand([this, Delta] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         int CurrentVolume = volume_ + Delta;

         if (CurrentVolume < 0)
         {
            CurrentVolume = 0;
         }
         else if (CurrentVolume > 100)
         {
            CurrentVolume = 100;
         }

         Debug("Client::Delta volume %d %d", Delta, CurrentVolume);

         if (mpd_run_set_volume(connection_, CurrentVolume) == true)
         {
            volume_ = CurrentVolume;
            EventData Data; Data.value = CurrentVolume;
            Main::Vimpc::CreateEvent(Event::Volume, Data);
         }
      }
   });
}

void Client::ToggleRandom()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Toggle random state %d", static_cast<int32_t>(!random_));

         if (mpd_run_random(connection_, !random_) == true)
         {
            SetStateAndEvent(Event::Random, random_, !random_);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::ToggleSingle()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Toggle single state %d", static_cast<int32_t>(!single_));

         if (mpd_run_single(connection_, !single_) == true)
         {
            SetStateAndEvent(Event::Single, single_, !single_);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::ToggleConsume()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Toggle consume state %d", static_cast<int32_t>(!consume_));

         if (mpd_run_consume(connection_, !consume_) == true)
         {
            SetStateAndEvent(Event::Consume, consume_, !consume_);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}


void Client::ToggleRepeat()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Toggle repeat state %d", static_cast<int32_t>(!repeat_));

         if (mpd_run_repeat(connection_, !repeat_) == true)
         {
            SetStateAndEvent(Event::Repeat, repeat_, !repeat_);
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::ToggleCrossfade()
{
   QueueCommand([this] ()
   {
      if (crossfade_ == false)
      {
         SetCrossfade(crossfadeTime_);
      }
      else
      {
         SetCrossfade(static_cast<uint32_t>(0));
      }
   });
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
            EventData Data; Data.name = name;
            Main::Vimpc::CreateEvent(Event::NewPlaylist, Data);
            Main::Vimpc::CreateEvent(Event::Repaint,   Data);
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

bool Client::HasPlaylist(std::string const & name)
{
   for (int i=0; i < lists_->Size(); i++) {
      if (lists_->Get(i).name_.c_str() == name)
      {
         return true;
      }
   }
   return false;
}

bool Client::HasLoadedPlaylist()
{
   return loadedList_ != "";
}

void Client::SaveLoadedPlaylist()
{
   if (Client::HasLoadedPlaylist())
   {
      Client::SavePlaylist(loadedList_);
   }
   else
   {
      ErrorString(ErrorNumber::NoPlaylistLoaded);
   }
}

void Client::SavePlaylist(std::string const & name)
{
   QueueCommand([this, name] ()
   {
      ClearCommand();

      if (Connected() == true)
      {

         if (HasPlaylist(name))
         {
            Debug("Client::Send remove %s", name.c_str());
            mpd_run_rm(connection_, name.c_str());
         }

         Debug("Client::Send save %s", name.c_str());
         if (mpd_run_save(connection_, name.c_str()) == true)
         {
            EventData Data; Data.name = name;
            Main::Vimpc::CreateEvent(Event::NewPlaylist, Data);
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
         loadedList_ = name;
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::AppendPlaylist(std::string const & name)
{
   QueueCommand([this, name] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Send load %s", name.c_str());
         mpd_run_load(connection_, name.c_str());
         loadedList_ = name;
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


void Client::PlaylistContents(std::string const & name)
{
   QueueCommand([this, name] ()
   {
      std::string const SongFormat = settings_.Get(Setting::SongFormat);

      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Add songs from playlist %s", name.c_str());

         mpd_send_list_playlist(connection_, name.c_str());

         mpd_song * nextSong = mpd_recv_song(connection_);

         std::vector<std::string> URIs;

         if (nextSong == NULL)
         {
            ErrorString(ErrorNumber::PlaylistEmpty);
         }
         else
         {
            for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
            {
               if ((settings_.Get(Setting::ListAllMeta) == false))
               {
                   Mpc::Song * song = Main::Library().Song(mpd_song_get_uri(nextSong));

                   if (song == NULL)
                   {
                       song = CreateSong(nextSong);
                       // Pre cache the print of the song
                       (void) song->FormatString(SongFormat);
                       EventData Data; Data.song = song;
                       Main::Vimpc::CreateEvent(Event::DatabaseSong, Data);
                   }
               }

               URIs.push_back(mpd_song_get_uri(nextSong));
               mpd_song_free(nextSong);
            }

            EventData Data; Data.name = name; Data.uris = URIs;
            Main::Vimpc::CreateEvent(Event::PlaylistContents, Data);
         }
      }
   });
}

void Client::PlaylistContentsForRemove(std::string const & name)
{
   QueueCommand([this, name] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Add songs from playlist %s", name.c_str());

         mpd_send_list_playlist(connection_, name.c_str());

         mpd_song * nextSong = mpd_recv_song(connection_);

         std::vector<std::string> URIs;

         if (nextSong != NULL)
         {
            for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
            {
               URIs.push_back(mpd_song_get_uri(nextSong));
               mpd_song_free(nextSong);
            }

            EventData Data; Data.name = name; Data.uris = URIs;
            Main::Vimpc::CreateEvent(Event::PlaylistContentsForRemove, Data);
         }
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
            Main::Vimpc::CreateEvent(Event::Repaint,   Data);
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
            Main::Vimpc::CreateEvent(Event::Repaint,   Data);
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

void Client::Add(std::vector<Mpc::Song *> songs)
{
   std::vector<std::string> URIs;

   for (auto song : songs)
   {
      URIs.push_back(song->URI());
   }

   QueueCommand([this, URIs] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         if (mpd_command_list_begin(connection_, false) == true)
         {
            listMode_ = true;

            Debug("Client::List add started");

            for (auto URI : URIs)
            {
               mpd_send_add(connection_, URI.c_str());
            }

            if (mpd_command_list_end(connection_) == true)
            {
               Debug("Client::List add success");
               listMode_ = false;

               for (auto URI : URIs)
               {
                  EventData Data; Data.uri = URI; Data.pos1 = -1;
                  Main::Vimpc::CreateEvent(Event::PlaylistAdd, Data);
               }

               EventData Data;
               Main::Vimpc::CreateEvent(Event::CommandListSend, Data);
               Main::Vimpc::CreateEvent(Event::Repaint,   Data);
            }
            else
            {
               CheckError();
            }
         }
         else
         {
            CheckError();
         }
      }
      else
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });
}

void Client::Add(Mpc::Song & song)
{
   std::string URI = song.URI();

   QueueCommand([this, URI] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         //Debug("Client::Add song %s", URI.c_str());
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
            ++currentSongId_;
            EventData IdData; IdData.id = currentSongId_;
            Main::Vimpc::CreateEvent(Event::CurrentSongId, IdData);
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
         mpd_send_add(connection_, "");
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

      // There might be an add in the queue, so we can't use the totalNumberOfSongs_ to determine
      // whether or not to do a delete
      if (Connected() == true) // && (totalNumberOfSongs_ > 0))
      {
         Debug("Client::Delete position %u", position);
         mpd_send_delete(connection_, position);

         if ((currentSongId_ > -1) && (position < static_cast<uint32_t>(currentSongId_)))
         {
            --currentSongId_;
            EventData IdData; IdData.id = currentSongId_;
            Main::Vimpc::CreateEvent(Event::CurrentSongId, IdData);
         }
      }
      else if (Connected() == false)
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });

   Main::Playlist().Remove(position, 1);
}

void Client::Delete(uint32_t position1, uint32_t position2)
{
   QueueCommand([this, position1, position2] ()
   {
      // There might be an add in the queue, so we can't use the totalNumberOfSongs_ to determine
      // whether or not to do a delete
      if (Connected() == true) // && (totalNumberOfSongs_ > 0))
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

                  EventData IdData; IdData.id = currentSongId_;
                  Main::Vimpc::CreateEvent(Event::CurrentSongId, IdData);
               }
            }
         }
      }

      if (Connected() == false)
      {
         ErrorString(ErrorNumber::ClientNoConnection);
      }
   });

   Main::Playlist().Remove(position1, position2 - position1);
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


void Client::AddAllSearchResults()
{
   QueueCommand([this] ()
   {
      if (Connected())
      {
         Mpc::Song Song;

         // Start the search
         Debug("Client::Add all search results");
         mpd_search_commit(connection_);

         // Recv the songs and do some callbacks
         mpd_song * nextSong = mpd_recv_song(connection_);

         if (nextSong == NULL)
         {
            ErrorString(ErrorNumber::FindNoResults);
         }
         else
         {
            for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
            {
               Song.SetURI(mpd_song_get_uri(nextSong));
               Add(Song);
               mpd_song_free(nextSong);
            }
         }
      }
   });
}

void Client::SearchResults(std::string const & name)
{
   QueueCommand([this, name] ()
   {
      std::string const SongFormat = settings_.Get(Setting::SongFormat);

      if (Connected())
      {
         Mpc::Song Song;

         // Start the search
         Debug("Client::Search results via events");
         mpd_search_commit(connection_);

         // Recv the songs and do some callbacks
         mpd_song * nextSong = mpd_recv_song(connection_);

         if (nextSong == NULL)
         {
            ErrorString(ErrorNumber::FindNoResults);
         }
         else
         {
            EventData Data; Data.name = name;

            for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
            {
               if ((settings_.Get(Setting::ListAllMeta) == false))
               {
                   Mpc::Song * song = Main::Library().Song(mpd_song_get_uri(nextSong));

                   if (song == NULL)
                   {
                       song = CreateSong(nextSong);
                       // Pre cache the print of the song
                       (void) song->FormatString(SongFormat);
                       EventData Data; Data.song = song;
                       Main::Vimpc::CreateEvent(Event::DatabaseSong, Data);
                   }
               }

               Data.uris.push_back(mpd_song_get_uri(nextSong));
               mpd_song_free(nextSong);
            }

            Main::Vimpc::CreateEvent(Event::SearchResults, Data);
         }
      }
   });
}


void Client::StateEvent()
{
   if (Connected() == true)
   {
      switch (state_)
      {
         case MPD_STATE_STOP:
            currentState_ = "Stopped";
            break;
         case MPD_STATE_PLAY:
            currentState_ = "Playing";
            break;
         case MPD_STATE_PAUSE:
            currentState_ = "Paused";
            break;
         case MPD_STATE_UNKNOWN:
         default:
            currentState_ = "Unknown";
            break;
      }
   }

   EventData Data; Data.clientstate = currentState_;
   Main::Vimpc::CreateEvent(Event::CurrentState, Data);
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

            EventData Data;
            Main::Vimpc::CreateEvent(Event::Update, Data);
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

            EventData Data;
            Main::Vimpc::CreateEvent(Event::Update, Data);
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
   bool const poll = settings_.Get(::Setting::Polling);

   if (time >= 0)
   {
      timeSinceUpdate_ += time;

      if (state_ == MPD_STATE_PLAY)
      {
         if (elapsed_ != (mpdelapsed_ + (timeSinceUpdate_ / 1000)))
         {
            elapsed_ = mpdelapsed_ + (timeSinceUpdate_ / 1000);

            EventData EData; EData.value = elapsed_;
            Main::Vimpc::CreateEvent(Event::Elapsed, EData);
         }
      }

      if ((poll == true) && (timeSinceUpdate_ > 900))
      {
         UpdateStatus();
      }
   }
   else
   {
      UpdateStatus();
   }
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

            EventData Data;
            Main::Vimpc::CreateEvent(Event::IdleMode, Data);
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

      EventData Data;
      Main::Vimpc::CreateEvent(Event::StopIdleMode, Data);
   }
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

         EventData Data;
         Main::Vimpc::CreateEvent(Event::StopIdleMode, Data);

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

   EventData IdData; IdData.id = currentSongId_;
   Main::Vimpc::CreateEvent(Event::CurrentSongId, IdData);

   if (autoscroll_ == true)
   {
      Main::Vimpc::CreateEvent(Event::Autoscroll, IdData);
      autoscroll_ = false;
   }

   if (currentSong_ != NULL)
   {
      EventData SongData; SongData.currentSong = mpd_song_dup(currentSong_);
      Main::Vimpc::CreateEvent(Event::CurrentSong, SongData);
   }
}

void Client::ClientQueueExecutor(Mpc::Client * client)
{
   struct timeval start, end;
   gettimeofday(&start, NULL);

   while (Running == true)
   {
      gettimeofday(&end,   NULL);
      long const seconds  = end.tv_sec  - start.tv_sec;
      long const useconds = end.tv_usec - start.tv_usec;
      long const mtime    = (seconds * 1000 + (useconds/1000.0)) + 0.5;
      IncrementTime(mtime);
      gettimeofday(&start, NULL);

      {
         UniqueLock<Mutex> Lock(QueueMutex);

         if ((Queue.empty() == false) ||
             (ConditionWait(Condition, Lock, 250) != false))
         {
            if (Queue.empty() == false)
            {
               FUNCTION<void()> function = Queue.front();
               Queue.pop_front();
               Lock.unlock();

               ExitIdleMode();
               function();
               continue;
            }
         }
      }

      {
         UniqueLock<Mutex> Lock(QueueMutex);

         if ((Queue.empty() == true) && (listMode_ == false))
         {
            if (queueUpdate_ == true)
            {
               Lock.unlock();
               QueueMetaChanges();
            }
            else if (idleMode_ == false)
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
	error_ = false;

   if ((listMode_ == false) && (idleMode_ == false) && (Connected() == true))
   {
      Debug("Client::Finish the response");
      mpd_response_finish(connection_);
      CheckError();
   }
}

bool Client::IsPasswordRequired()
{
   Debug("Client::Trying status update to determine if password required");

   bool Result = false;

   if (Connected() == true)
   {
      struct mpd_status * status = mpd_run_status(connection_);

      mpd_error error = mpd_connection_get_error(connection_);

      if (error != MPD_ERROR_SUCCESS)
      {
         if ((error == MPD_ERROR_SERVER) &&
            (mpd_connection_get_server_error(connection_) == MPD_SERVER_ERROR_PERMISSION))
         {
            Result = true;
         }

         mpd_connection_clear_error(connection_);
      }

      if (status != NULL)
      {
         mpd_status_free(status);
      }
   }

   return Result;
}


void Client::GetAllMetaInformation()
{
   ClearCommand();

   std::string const SongFormat = settings_.Get(Setting::SongFormat);

   std::vector<Mpc::Song *> songs;
   std::vector<std::string> paths;
   std::vector<std::pair<std::string, std::string> > lists;

   if (Connected() == true)
   {
      EventData DBData;
      Main::Vimpc::CreateEvent(Event::ClearDatabase, DBData);

      Debug("Client::Get all meta information");

      if ((settings_.Get(Setting::ListAllMeta) == true))
      {
          mpd_send_list_all_meta(connection_, NULL);

          mpd_entity * nextEntity = mpd_recv_entity(connection_);

          for(; nextEntity != NULL; nextEntity = mpd_recv_entity(connection_))
          {
             if (mpd_entity_get_type(nextEntity) == MPD_ENTITY_TYPE_SONG)
             {
                mpd_song const * const nextSong = mpd_entity_get_song(nextEntity);

                if (nextSong != NULL)
                {
                   Song * const newSong = CreateSong(nextSong);
                   songs.push_back(newSong);
                }
             }
             else if (mpd_entity_get_type(nextEntity) == MPD_ENTITY_TYPE_DIRECTORY)
             {
                mpd_directory const * const nextDirectory = mpd_entity_get_directory(nextEntity);
                paths.push_back(std::string(mpd_directory_get_path(nextDirectory)));
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

                   lists.push_back(std::make_pair(name, path));
                }
             }

             mpd_entity_free(nextEntity);
          }
       }
   }

   ClearCommand();

   if (error_)
   {
       Debug("List all failed, disabling\n");
       Error(ErrorNumber::ErrorClear, "");
       ErrorString(ErrorNumber::ClientNoMeta, "not supported on server");
       settings_.Set(Setting::ListAllMeta, false);
   }

   EventData DatabaseEvent;
   DatabaseEvent.state = (settings_.Get(Setting::ListAllMeta));
   Main::Vimpc::CreateEvent(Event::DatabaseEnabled, DatabaseEvent);

   if (Connected() == true)
   {
      // Songs, paths, lists, etc are collated and the events created this way
      // because mpd seems to disconnect you if you take to long to recv entities
      for (auto song : songs)
      {
         // Pre cache the print of the song
         (void) song->FormatString(SongFormat);
         EventData Data; Data.song = song;
         Main::Vimpc::CreateEvent(Event::DatabaseSong, Data);
      }

      for (auto path : paths)
      {
         EventData Data; Data.uri = path;
         Main::Vimpc::CreateEvent(Event::DatabasePath, Data);
      }

      for (auto list : lists)
      {
         EventData Data; Data.name = list.first; Data.uri = list.second;
         Main::Vimpc::CreateEvent(Event::DatabaseListFile, Data);
      }
   }

   songs.clear();

   if (Connected() == true)
   {
      Debug("Client::List queue meta data");
      mpd_send_list_queue_meta(connection_);

      mpd_song * nextSong = mpd_recv_song(connection_);

      for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
      {
         EventData Data; Data.song = NULL; Data.uri = mpd_song_get_uri(nextSong); Data.pos1 = -1;

         if (((settings_.Get(Setting::ListAllMeta) == false) &&
              (Main::Library().Song(Data.uri) == NULL)) ||
             // Handle "virtual" songs embedded within files
             (mpd_song_get_end(nextSong) != 0))
         {
            Song * const newSong = CreateSong(nextSong);
            Data.song = newSong;

            if (settings_.Get(Setting::ListAllMeta) == false) {
               songs.push_back(newSong);
            }
         }

         Main::Vimpc::CreateEvent(Event::PlaylistAdd, Data);

         mpd_song_free(nextSong);
      }
   }

   if (settings_.Get(Setting::ListAllMeta) == false) {
      if (Connected() == true)
      {
         // Songs, paths, lists, etc are collated and the events created this way
         // because mpd seems to disconnect you if you take to long to recv entities
         for (auto song : songs)
         {
            // Pre cache the print of the song
            (void) song->FormatString(SongFormat);
            EventData Data; Data.song = song;
            Main::Vimpc::CreateEvent(Event::DatabaseSong, Data);
         }
      }
   }

   if (Connected() == true)
   {
#if LIBMPDCLIENT_CHECK_VERSION(2,5,0)
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Request playlists");

         if (mpd_send_list_playlists(connection_))
         {
            mpd_playlist * nextPlaylist = mpd_recv_playlist(connection_);

            for(; nextPlaylist != NULL; nextPlaylist = mpd_recv_playlist(connection_))
            {
               std::string const playlist = mpd_playlist_get_path(nextPlaylist);

               EventData Data; Data.uri = playlist; Data.name = playlist;
               Main::Vimpc::CreateEvent(Event::DatabaseList, Data);

               mpd_playlist_free(nextPlaylist);
            }
         }

         mpd_connection_clear_error(connection_);
      }
#endif
   }

   if (Connected() == true)
   {
      EventData Data;
      Main::Vimpc::CreateEvent(Event::AllMetaDataReady, Data);
      Main::Vimpc::CreateEvent(Event::Repaint,   Data);
   }

#if !LIBMPDCLIENT_CHECK_VERSION(2,5,0)
   GetAllMetaFromRoot();
#endif
}

void Client::GetAllOutputs()
{
   QueueCommand([this] ()
   {
      ClearCommand();

      if (Connected() == true)
      {
         Debug("Client::Get outputs");
         mpd_send_outputs(connection_);

         mpd_output * next = mpd_recv_output(connection_);

         for (; next != NULL; next = mpd_recv_output(connection_))
         {
            Mpc::Output * output = new Mpc::Output(mpd_output_get_id(next));

            output->SetEnabled(mpd_output_get_enabled(next));
            output->SetName(mpd_output_get_name(next));

            EventData Data; Data.output = output;
            Main::Vimpc::CreateEvent(Event::Output, Data);

            mpd_output_free(next);
         }

         Debug("Client::Get outputs complete");
         EventData Data;
         Main::Vimpc::CreateEvent(Event::Repaint,Data);
      }
   });
}

void Client::GetAllMetaFromRoot()
{
   // This is a hack to get playlists when using older libmpdclients, it should
   // not be used unless absolutely necessary
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

               EventData Data; Data.uri = path; Data.name = name;
               Main::Vimpc::CreateEvent(Event::DatabaseList, Data);
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
      ClearCommand();

      if ((Connected() == true) && (listMode_ == false))
      {
         Debug("Client::Start command list");

         if (mpd_command_list_begin(connection_, false) == true)
         {
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
            listMode_ = false;
            EventData Data;
            Main::Vimpc::CreateEvent(Event::CommandListSend, Data);
            Main::Vimpc::CreateEvent(Event::Repaint,   Data);
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


void Client::SetStateAndEvent(int event, bool & state, bool value)
{
   state = value;
   EventData Data; Data.state = value;
   Main::Vimpc::CreateEvent(event, Data);
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
            mpd_status_free(currentStatus_);
            currentStatus_ = NULL;
         }

         Debug("Client::Get current status");
         struct mpd_status * status = mpd_run_status(connection_);
         CheckError();

         if (status != NULL)
         {
            currentStatus_   = status;
            timeSinceUpdate_ = 0;

            unsigned int version     = mpd_status_get_queue_version(currentStatus_);
            unsigned int qVersion    = static_cast<uint32_t>(queueVersion_);
            bool const   wasUpdating = updating_;

            if (static_cast<int32_t>(volume_) != mpd_status_get_volume(currentStatus_))
            {
               volume_ = mpd_status_get_volume(currentStatus_);

               EventData Data; Data.value = volume_;
               Main::Vimpc::CreateEvent(Event::Volume, Data);
            }

            if (updating_ != (mpd_status_get_update_id(currentStatus_) >= 1))
            {
               updating_ = (mpd_status_get_update_id(currentStatus_) >= 1);

               if (updating_ == true)
               {
                  EventData Data;
                  Main::Vimpc::CreateEvent(Event::Update, Data);
               }
            }

            if (random_ != mpd_status_get_random(currentStatus_))
            {
               SetStateAndEvent(Event::Random, random_, mpd_status_get_random(currentStatus_));
            }

            if (repeat_ != mpd_status_get_repeat(currentStatus_))
            {
               SetStateAndEvent(Event::Repeat, repeat_, mpd_status_get_repeat(currentStatus_));
            }

            if (single_ != mpd_status_get_single(currentStatus_))
            {
               SetStateAndEvent(Event::Single, single_, mpd_status_get_single(currentStatus_));
            }

            if (consume_ != mpd_status_get_consume(currentStatus_))
            {
               SetStateAndEvent(Event::Consume, consume_, mpd_status_get_consume(currentStatus_));
            }

            if (totalNumberOfSongs_ != mpd_status_get_queue_length(currentStatus_))
            {
               totalNumberOfSongs_ = mpd_status_get_queue_length(currentStatus_);

               EventData Data; Data.count = totalNumberOfSongs_;
               Main::Vimpc::CreateEvent(Event::TotalSongCount, Data);
            }

            if (crossfade_ != (mpd_status_get_crossfade(currentStatus_) > 0))
            {
               crossfade_ = (mpd_status_get_crossfade(currentStatus_) > 0);
               EventData Data; Data.state = crossfade_;
               Main::Vimpc::CreateEvent(Event::Crossfade, Data);
            }

            if (crossfade_ == true)
            {
               if (crossfadeTime_ != mpd_status_get_crossfade(currentStatus_))
               {
                  crossfadeTime_ = mpd_status_get_crossfade(currentStatus_);
                  EventData Data; Data.value = crossfadeTime_;
                  Main::Vimpc::CreateEvent(Event::CrossfadeTime, Data);
               }
            }

            // Check if we need to update the current song
            if ((mpdstate_ != mpd_status_get_state(currentStatus_)) ||
               ((mpdstate_ != MPD_STATE_STOP) && (currentSong_ == NULL)) ||
               ((mpdstate_ == MPD_STATE_STOP) && (currentSong_ != NULL)) ||
               ((currentSong_ != NULL) &&
                (mpd_song_get_duration(currentSong_) > 0) &&
                ((elapsed_ >= mpd_song_get_duration(currentSong_) - 3) ||
                  (mpd_status_get_elapsed_time(currentStatus_) < mpdelapsed_) ||
                  (mpd_status_get_elapsed_time(currentStatus_) <= 3))))
            {
               UpdateCurrentSong();
            }

            mpdstate_   = mpd_status_get_state(currentStatus_);
            mpdelapsed_ = mpd_status_get_elapsed_time(currentStatus_);

            if (state_ != mpdstate_)
            {
               state_ = mpdstate_;
               StateEvent();
            }

            if (mpdstate_ == MPD_STATE_STOP)
            {
               currentSongId_  = -1;
               currentSongURI_ = "";

               EventData IdData; IdData.id = currentSongId_;
               Main::Vimpc::CreateEvent(Event::CurrentSongId, IdData);
               EventData Data; Data.currentSong = NULL;
               Main::Vimpc::CreateEvent(Event::CurrentSong, Data);
            }

            if (mpdstate_ != MPD_STATE_PLAY)
            {
               elapsed_ = mpdelapsed_;
            }

            EventData EData; EData.value = elapsed_;
            Main::Vimpc::CreateEvent(Event::Elapsed, EData);

            if ((queueVersion_ > -1) && (version > qVersion) && (queueUpdate_ == false))
            {
               oldVersion_  = queueVersion_;
               queueUpdate_ = true;
            }

            if ((wasUpdating == true) && (updating_ == false))
            {
               GetAllMetaInformation();
               UpdateCurrentSong();

               EventData Data;
               Main::Vimpc::CreateEvent(Event::UpdateComplete, Data);
               Main::Vimpc::CreateEvent(Event::Repaint,   Data);
            }

            queueVersion_ = version;
         }
      }
   });
}

Song * Client::CreateSong(mpd_song const * const song) const
{
   static int count = 0;

   Song * const newSong = new Song();

   //Debug("Alloc a song %d %d", count, sizeof(Song));

   newSong->SetArtist     (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
   newSong->SetAlbumArtist(mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0));
   newSong->SetAlbum      (mpd_song_get_tag(song, MPD_TAG_ALBUM,  0));
   newSong->SetTitle      (mpd_song_get_tag(song, MPD_TAG_TITLE,  0));
   newSong->SetTrack      (mpd_song_get_tag(song, MPD_TAG_TRACK,  0));
   newSong->SetURI        (mpd_song_get_uri(song));
   newSong->SetGenre      (mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
   newSong->SetDate       (mpd_song_get_tag(song, MPD_TAG_DATE, 0));
   newSong->SetDisc       (mpd_song_get_tag(song, MPD_TAG_DISC, 0));
   newSong->SetDuration   (mpd_song_get_duration(song));
   newSong->SetVirtualEnd (mpd_song_get_end(song));

   return newSong;
}

void Client::QueueMetaChanges()
{
   ClearCommand();

   if (Connected() == true)
   {
      struct mpd_status * status = mpd_run_status(connection_);
      queueVersion_ = mpd_status_get_queue_version(status);

      if (totalNumberOfSongs_ != mpd_status_get_queue_length(status))
      {
         totalNumberOfSongs_ = mpd_status_get_queue_length(status);

         EventData Data; Data.count = totalNumberOfSongs_;
         Main::Vimpc::CreateEvent(Event::TotalSongCount, Data);
      }

      if (oldVersion_ != queueVersion_)
      {
         Debug("Client::List queue meta data changes %d %d", oldVersion_, queueVersion_);
         mpd_send_queue_changes_meta(connection_, oldVersion_);

         mpd_song * nextSong = mpd_recv_song(connection_);
         EventData Data;
         Data.count = totalNumberOfSongs_;

         for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
         {
            Song * newSong = NULL;

            if (((settings_.Get(Setting::ListAllMeta) == false) &&
                 (Main::Library().Song(Data.uri) == NULL)) ||
                // Handle "virtual" songs embedded within files
                (mpd_song_get_end(nextSong) != 0))
            {
               newSong = CreateSong(nextSong);
            }

            //Debug("Change: %d %s", mpd_song_get_pos(nextSong), mpd_song_get_uri(nextSong));
            Data.posuri.push_back(std::make_pair(mpd_song_get_pos(nextSong), std::make_pair(newSong, mpd_song_get_uri(nextSong))));
            mpd_song_free(nextSong);
         }

         QueueMutex.lock();

         if (Queue.size() == 0)
         {
            QueueMutex.unlock();

            oldVersion_  = queueVersion_;
            queueUpdate_ = false;
            Main::Vimpc::CreateEvent(Event::PlaylistQueueReplace, Data);

            EventData QueueData;
            Main::Vimpc::CreateEvent(Event::QueueUpdate, QueueData);

            UpdateCurrentSong();
         }
         else
         {
            QueueMutex.unlock();
         }
      }
      else
      {
         queueUpdate_ = false;
      }
   }
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
         error_ = true;

         bool const ClearError = mpd_connection_clear_error(connection_);

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
   state_        = MPD_STATE_UNKNOWN;

   totalNumberOfSongs_ = 0;

   versionMajor_ = -1;
   versionMinor_ = -1;
   versionPatch_ = -1;
   queueVersion_ = -1;

   StateEvent();

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

   EventData Data;
   Main::Vimpc::CreateEvent(Event::Disconnected, Data);

   ENSURE(connection_ == NULL);
}


/* vim: set sw=3 ts=3: */
