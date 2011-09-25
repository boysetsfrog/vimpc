/*
   Vimpc
   Copyright (C) 2010 - 2011 Nathan Sweetman

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
   class Output;
   class Song;

   class Client
   {
   public:
      Client(Main::Vimpc * vimpc, Main::Settings & settings, Ui::Screen & screen);
      ~Client();

   private:
      Client(Client & client);
      Client & operator=(Client & client);

   public:
      void Connect(std::string const & hostname = "", uint16_t port = 0);
      std::string Hostname();
      uint16_t Port();

   public:
      void StartCommandList();
      void SendCommandList();

   public:
      void Play(uint32_t playId);
      void Pause();
      void Stop();
      void Next();
      void Previous();

   public:
      bool Random();
      void SetRandom(bool random);

      bool Single();
      void SetSingle(bool single);

      bool Consume();
      void SetConsume(bool consume);

      bool Repeat();
      void SetRepeat(bool repeat);

      int32_t Volume();
      void SetVolume(uint32_t volume);

   public:
      void Shuffle();
      void Move(uint32_t position1, uint32_t position2);
      void Swap(uint32_t position1, uint32_t position2);

   public:
      void CreatePlaylist(std::string const & name);
      void SavePlaylist(std::string const & name);
      void LoadPlaylist(std::string const & name);
      void RemovePlaylist(std::string const & name);
      void AddToPlaylist(std::string const & name, Mpc::Song * song);

   public:
      void EnableOutput(Mpc::Output * output);
      void DisableOutput(Mpc::Output * output);

   public: //Queue
      uint32_t Add(Mpc::Song & song);
      uint32_t Add(Mpc::Song & song, uint32_t position);
      uint32_t AddAllSongs();
      void Add(Mpc::Song * song);
      void Delete(uint32_t position);
      void Delete(uint32_t position1, uint32_t position2);
      void Clear();

      void Rescan();
      void Update();

   public:
      void CheckForUpdates();

   public:
      std::string CurrentState();
      bool Connected() const;

   public:
      std::string GetCurrentSongURI() ;
      int32_t  GetCurrentSong();
      uint32_t TotalNumberOfSongs();

   public:
      void SearchAny(std::string const & search, bool exact = false);
      void SearchArtist(std::string const & search, bool exact = false);
      void SearchAlbum(std::string const & search, bool exact = false);
      void SearchSong(std::string const & search, bool exact = false);

   public:
      bool SongIsInQueue(Mpc::Song const & song) const;
      void DisplaySongInformation();

   public:
      //! \todo port these over to using the callback object
      template <typename Object>
      void ForEachQueuedSong(Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachLibrarySong(Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachPlaylistSong(std::string Playlist, Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachPlaylist(Object & object, void (Object::*callBack)(std::string));

      template <typename Object>
      void ForEachSearchResult(Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachOutput(Object & object, void (Object::*callBack)(Mpc::Output *));

   private:
      unsigned int QueueVersion();
      Song * CreateSong(uint32_t id, mpd_song const * const) const;

   private:
      void CheckError();
      void DeleteConnection();

   private:
      Main::Vimpc *           vimpc_;
      Main::Settings &        settings_;
      struct mpd_connection * connection_;

      std::string             hostname_;
      uint16_t                port_;

      struct mpd_song *       currentSong_;
      struct mpd_status *     currentStatus_;
      int32_t                 currentSongId_;
      std::string             currentSongURI_;
      std::string             currentState_;

      Ui::Screen &            screen_;
      unsigned int            queueVersion_;
      bool                    forceUpdate_;
      bool                    listMode_;
   };

   //
   template <typename Object>
   void Client::ForEachQueuedSong(Object & object, void (Object::*callBack)(Mpc::Song *))
   {
      if (Connected() == true)
      {
         queueVersion_ = QueueVersion();

         mpd_send_list_queue_meta(connection_);

         mpd_song * nextSong = mpd_recv_song(connection_);

         for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
         {
            uint32_t const     position = mpd_song_get_pos(nextSong);
            Song const * const newSong  = CreateSong(position, nextSong);
            Song * const       oldSong  = Main::Library().Song(newSong);

            if (oldSong != NULL)
            {
               (object.*callBack)(oldSong);
            }

            mpd_song_free(nextSong);
            delete newSong;
         }
      }
   }

   //
   template <typename Object>
   void Client::ForEachLibrarySong(Object & object, void (Object::*callBack)(Mpc::Song * ))
   {
      if (Connected() == true)
      {
         mpd_send_list_all_meta(connection_, "/");

         mpd_entity * nextEntity = mpd_recv_entity(connection_);

         for(; nextEntity != NULL; nextEntity = mpd_recv_entity(connection_))
         {
            if (mpd_entity_get_type(nextEntity) == MPD_ENTITY_TYPE_SONG)
            {
               mpd_song const * const nextSong = mpd_entity_get_song(nextEntity);

               if (nextSong != NULL)
               {
                  Song * const newSong = CreateSong(-1, nextSong);

                  (object.*callBack)(newSong);

                  delete newSong;
               }
            }
            mpd_entity_free(nextEntity);
         }
      }
   }

   //
   template <typename Object>
   void Client::ForEachPlaylistSong(std::string playlist, Object & object, void (Object::*callBack)(Mpc::Song * ))
   {
#if LIBMPDCLIENT_CHECK_VERSION(2,5,0)
      if (Connected() == true)
      {
         mpd_send_list_playlist_meta(connection_, playlist.c_str());

         mpd_song * nextSong = mpd_recv_song(connection_);

         for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
         {
            uint32_t const     position = mpd_song_get_pos(nextSong);
            Song const * const newSong  = CreateSong(position, nextSong);
            Song * const       oldSong  = Main::Library().Song(newSong);

            if (oldSong != NULL)
            {
               (object.*callBack)(oldSong);
            }

            mpd_song_free(nextSong);
            delete newSong;
         }
      }
#endif
   }

   //
   template <typename Object>
   void Client::ForEachPlaylist(Object & object, void (Object::*callBack)(std::string))
   {
#if LIBMPDCLIENT_CHECK_VERSION(2,5,0)
      if (Connected() == true)
      {
         mpd_send_list_playlists(connection_);

         mpd_playlist * nextPlaylist = mpd_recv_playlist(connection_);

         for(; nextPlaylist != NULL; nextPlaylist = mpd_recv_playlist(connection_))
         {
            std::string const playlist = mpd_playlist_get_path(nextPlaylist);
            (object.*callBack)(playlist);
            mpd_playlist_free(nextPlaylist);
         }
      }
#endif
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
            uint32_t const     position = mpd_song_get_pos(nextSong);
            Song const * const newSong  = CreateSong(position, nextSong);
            Song * const       oldSong  = Main::Library().Song(newSong);

            if (oldSong != NULL)
            {
               (object.*callBack)(oldSong);
            }

            mpd_song_free(nextSong);
            delete newSong;
         }
      }
   }

   template <typename Object>
   void Client::ForEachOutput(Object & object, void (Object::*callBack)(Mpc::Output *))
   {
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
