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

   mpdclient.hpp - provides interaction with the music player daemon
   */

#ifndef __MPC__CLIENT
#define __MPC__CLIENT

#include <mpd/client.h>

#include "output.hpp"
#include "screen.hpp"
#include "buffers.hpp"
#include "buffer/library.hpp"
#include "buffer/list.hpp"

// The library check in 2.1.0 doesn't seem to work
// since we don't support versions older than that anyway, just return false
// instead of using the check macro
#if ((LIBMPDCLIENT_MAJOR_VERSION == 2) && (LIBMPDCLIENT_MINOR_VERSION == 1))
#undef LIBMPDCLIENT_CHECK_VERSION
#define LIBMPDCLIENT_CHECK_VERSION(major, minor, patch) 0
#endif

namespace Main
{
   class Settings;
   class Vimpc;
}

namespace Ui
{
   class Screen;
}

// \todo cache all the values that we can
namespace Mpc
{
   class Client;
   class Output;
   class Song;

   uint32_t SecondsToMinutes(uint32_t duration);
   uint32_t RemainingSeconds(uint32_t duration);

   class CommandList
   {
      public:
         CommandList(Mpc::Client & client, bool condition = true);
         ~CommandList();

      private:
         bool          condition_;
         bool          list_;
         Mpc::Client & client_;
   };

   class Client
   {
      friend class Mpc::CommandList;

   public:
      Client(Main::Vimpc * vimpc, Main::Settings & settings, Ui::Screen & screen);
      ~Client();

   private:
      Client(Client & client);
      Client & operator=(Client & client);

   public:
      // Mpd Connections
      void Connect(std::string const & hostname = "", uint16_t port = 0, uint32_t timeout_ms = 0);
      void Disconnect();
      void Reconnect();
      void Password(std::string const & password);

      std::string Hostname();
      uint16_t Port();
      bool Connected() const;

   public:
      // Playback functions
      void Play(uint32_t playId);
      void Pause();
      void Stop();
      void Next();
      void Previous();
      void Seek(int32_t Offset);
      void SeekTo(uint32_t Time);
      void SeekToPercent(double Percent);

   public:
      // Toggle settings
      bool Random();
      void SetRandom(bool random);

      bool Single();
      void SetSingle(bool single);

      bool Consume();
      void SetConsume(bool consume);

      bool Repeat();
      void SetRepeat(bool repeat);

      int32_t Crossfade();
      void SetCrossfade(bool crossfade);
      void SetCrossfade(uint32_t crossfade);

      int32_t Volume();
      void SetVolume(uint32_t volume);

      bool IsUpdating();

   public:
      // Playlist editing
      void Shuffle();
      void Move(uint32_t position1, uint32_t position2);
      void Swap(uint32_t position1, uint32_t position2);

   public:
      // Playlist management
      void CreatePlaylist(std::string const & name);
      void SavePlaylist(std::string const & name);
      void LoadPlaylist(std::string const & name);
      void RemovePlaylist(std::string const & name);
      void AddToNamedPlaylist(std::string const & name, Mpc::Song * song);

   public:
      // Outputs
      void SetOutput(Mpc::Output * output, bool enable);
      void EnableOutput(Mpc::Output * output);
      void DisableOutput(Mpc::Output * output);

   public:
      // Queue manipulation
      void Add(Mpc::Song * song);
      uint32_t Add(Mpc::Song & song);
      uint32_t Add(Mpc::Song & song, uint32_t position);
      uint32_t AddAllSongs();

      //! This add is only used by a command when a full uri is specified
      //! it should not be used to add songs from the library, you should use the
      //! versions above for that purpose
      uint32_t Add(std::string const & URI);

      // Call after all songs have been added
      void AddComplete();

      void Delete(uint32_t position);
      void Delete(uint32_t position1, uint32_t position2);
      void Clear();

   public:
      // Searching the database
      void SearchAny(std::string const & search, bool exact = false);
      void SearchArtist(std::string const & search, bool exact = false);
      void SearchAlbum(std::string const & search, bool exact = false);
      void SearchGenre(std::string const & search, bool exact = false);
      void SearchSong(std::string const & search, bool exact = false);

   public:
      // Mpd Status
      std::string CurrentState();
      std::string GetCurrentSongURI() ;

      int32_t  GetCurrentSong();
      uint32_t TotalNumberOfSongs();

      bool SongIsInQueue(Mpc::Song const & song) const;
      void DisplaySongInformation();

   public:
      // Database state
      void Rescan(std::string const & Path);
      void Update(std::string const & Path);
      void IncrementTime(long time);
      long TimeSinceUpdate();
      void IdleMode();
      bool IsIdle();
      bool IsCommandList();
      void StartCommandList();
      void SendCommandList();
      bool HadEvents();
      void UpdateCurrentSong();
      void UpdateStatus(bool ExpectUpdate = false);
      void UpdateDisplay();

   public:
      //! \todo port these over to using the callback object
      template <typename Object>
      void ForEachQueuedSong(Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachQueuedSongChanges(uint32_t oldVersion, Object & object, void (Object::*callBack)(uint32_t, Mpc::Song *));

      template <typename Object>
      void ForEachLibrarySong(Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachDirectory(Object & object, void (Object::*callBack)(std::string));

      template <typename Object>
      void ForEachPlaylistSong(std::string Playlist, Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachPlaylist(Object & object, void (Object::*callBack)(Mpc::List));

      template <typename Object>
      void ForEachPlaylistEntity(Object & object, void (Object::*callBack)(Mpc::List));

      template <typename Object>
      void ForEachSearchResult(Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachOutput(Object & object, void (Object::*callBack)(Mpc::Output *));

      void GetAllMetaInformation();

   private:
      void ClearCommand();
      bool Command(bool InputCommand);

   private:
      unsigned int QueueVersion();
      void UpdateCurrentSongPosition();
      Song * CreateSong(uint32_t id, mpd_song const * const, bool songInLibrary = true) const;

   private:
      void GetVersion();
      bool CheckError();
      void DeleteConnection();

   private:
      Main::Vimpc *           vimpc_;
      Main::Settings &        settings_;
      struct mpd_connection * connection_;
      int                     fd_;

      std::string             hostname_;
      uint16_t                port_;
      uint32_t                versionMajor_;
      uint32_t                versionMinor_;
      uint32_t                versionPatch_;
      long                    timeSinceUpdate_;
      long                    timeSinceSong_;
      bool                    retried_;

      uint32_t                volume_;
      bool                    updating_;
      bool                    random_;
      bool                    repeat_;
      bool                    single_;
      bool                    consume_;
      bool                    crossfade_;
      uint32_t                crossfadeTime_;
      uint32_t                elapsed_;
      uint32_t                mpdelapsed_;
      mpd_state               state_;
      mpd_state               mpdstate_;

      struct mpd_song *       currentSong_;
      struct mpd_status *     currentStatus_;
      int32_t                 currentSongId_;
      std::string             currentSongURI_;
      std::string             currentState_;

      Ui::Screen &            screen_;
      int                     queueVersion_;
      bool                    forceUpdate_;
      bool                    listMode_;
      bool                    idleMode_;
      bool                    hadEvents_;

      std::vector<Mpc::Song *> songs_;
      std::vector<std::string> paths_;
      std::vector<Mpc::List>   playlists_;
   };

   //
   template <typename Object>
   void Client::ForEachQueuedSong(Object & object, void (Object::*callBack)(Mpc::Song *))
   {
      ClearCommand();

      if (Connected() == true)
      {
         mpd_send_list_queue_meta(connection_);

         mpd_song * nextSong = mpd_recv_song(connection_);

         for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
         {
            Song * song = Main::Library().Song(mpd_song_get_uri(nextSong));

            if (song == NULL)
            {
               song = CreateSong(-1, nextSong);
            }

            if (song != NULL)
            {
               (object.*callBack)(song);
            }

            mpd_song_free(nextSong);
         }
      }
   }

   //
   template <typename Object>
   void Client::ForEachQueuedSongChanges(uint32_t oldVersion, Object & object, void (Object::*callBack)(uint32_t, Mpc::Song *))
   {
      ClearCommand();

      if (Connected() == true)
      {
         mpd_send_queue_changes_meta(connection_, oldVersion);

         mpd_song * nextSong = mpd_recv_song(connection_);

         for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
         {
            uint32_t const position = mpd_song_get_pos(nextSong);
            Song *         song     = Main::Library().Song(mpd_song_get_uri(nextSong));

            if (song == NULL)
            {
               song = CreateSong(-1, nextSong, false);
            }

            if (song != NULL)
            {
               (object.*callBack)(position, song);
            }

            mpd_song_free(nextSong);
         }
      }
   }

   //
   template <typename Object>
   void Client::ForEachLibrarySong(Object & object, void (Object::*callBack)(Mpc::Song * ))
   {
      for (std::vector<Mpc::Song *>::iterator it = songs_.begin(); it != songs_.end(); ++it)
      {
         (object.*callBack)(*it);
      }
   }

   //
   template <typename Object>
   void Client::ForEachDirectory(Object & object, void (Object::*callBack)(std::string))
   {
      for (std::vector<std::string>::iterator it = paths_.begin(); it != paths_.end(); ++it)
      {
         (object.*callBack)(*it);
      }
   }

   //
   template <typename Object>
   void Client::ForEachPlaylistSong(std::string playlist, Object & object, void (Object::*callBack)(Mpc::Song * ))
   {
      ClearCommand();

      if (Connected() == true)
      {
         mpd_send_list_playlist(connection_, playlist.c_str());

         mpd_song * nextSong = mpd_recv_song(connection_);

         for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
         {
            Song * const song = Main::Library().Song(mpd_song_get_uri(nextSong));

            if (song != NULL)
            {
               (object.*callBack)(song);
            }

            mpd_song_free(nextSong);
         }
      }
   }

   //
   template <typename Object>
   void Client::ForEachPlaylist(Object & object, void (Object::*callBack)(Mpc::List))
   {
#if LIBMPDCLIENT_CHECK_VERSION(2,5,0)
      if ((settings_.Get(Setting::Playlists) == Setting::PlaylistsAll) ||
         (settings_.Get(Setting::Playlists) == Setting::PlaylistsMpd))
      {
         ClearCommand();

         if (Connected() == true)
         {
            mpd_send_list_playlists(connection_);

            mpd_playlist * nextPlaylist = mpd_recv_playlist(connection_);

            for(; nextPlaylist != NULL; nextPlaylist = mpd_recv_playlist(connection_))
            {
               std::string const playlist = mpd_playlist_get_path(nextPlaylist);
               (object.*callBack)(Mpc::List(playlist));
               mpd_playlist_free(nextPlaylist);
            }
         }
      }
#endif

      if ((settings_.Get(Setting::Playlists) == Setting::PlaylistsAll) ||
         (settings_.Get(Setting::Playlists) == Setting::PlaylistsFiles))
      {
         for (std::vector<Mpc::List>::iterator it = playlists_.begin(); it != playlists_.end(); ++it)
         {
            (object.*callBack)(*it);
         }
      }
   }

   //
   template <typename Object>
   void Client::ForEachPlaylistEntity(Object & object, void (Object::*callBack)(Mpc::List))
   {
      for (std::vector<Mpc::List>::iterator it = playlists_.begin(); it != playlists_.end(); ++it)
      {
         (object.*callBack)(*it);
      }
   }

   // Requires search to be prepared before calling
   template <typename Object>
   void Client::ForEachSearchResult(Object & object, void (Object::*callBack)(Mpc::Song * ))
   {
      if (Connected())
      {
         // Start the search
         mpd_search_commit(connection_);

         // Recv the songs and do some callbacks
         mpd_song * nextSong = mpd_recv_song(connection_);

         for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
         {
            Song * const song = Main::Library().Song(mpd_song_get_uri(nextSong));

            if (song != NULL)
            {
               (object.*callBack)(song);
            }

            mpd_song_free(nextSong);
         }
      }
   }

   template <typename Object>
   void Client::ForEachOutput(Object & object, void (Object::*callBack)(Mpc::Output *))
   {
      ClearCommand();

      if (Connected() == true)
      {
         mpd_send_outputs(connection_);

         mpd_output * next = mpd_recv_output(connection_);

         for (; next != NULL; next = mpd_recv_output(connection_))
         {
            Mpc::Output * output = new Mpc::Output(mpd_output_get_id(next));

            output->SetEnabled(mpd_output_get_enabled(next));
            output->SetName(mpd_output_get_name(next));

            (object.*callBack)(output);

            mpd_output_free(next);
         }
      }
   }
}

#endif
/* vim: set sw=3 ts=3: */
