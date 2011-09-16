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

   mpdclient.hpp - provides interaction with the music player daemon
   */

#ifndef __MPC__CLIENT
#define __MPC__CLIENT

#include <mpd/client.h>

#include "screen.hpp"
#include "buffers.hpp"
#include "buffer/library.hpp"

namespace Main
{
   class Vimpc;
}

namespace Ui
{
   class Screen;
}

// \todo cache all the values that we can
namespace Mpc
{
   class Song;

   class Client
   {
   public:
      Client(Main::Vimpc * vimpc, Ui::Screen const & screen);
      ~Client();

   private:
      Client(Client & client);
      Client & operator=(Client & client);

   public:
      void Connect(std::string const & hostname = "", uint16_t port = 0);
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
      void SavePlaylist(std::string const & name);
      void LoadPlaylist(std::string const & name);

   public: //Queue
      uint32_t Add(Mpc::Song & song);
      uint32_t Add(Mpc::Song & song, uint32_t position);
      uint32_t AddAllSongs();
      void Delete(uint32_t position);
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
      bool SongIsInQueue(Mpc::Song const & song) const;
      void DisplaySongInformation();

   public:
      //! \todo port these over to using the callback object
      template <typename Object>
      void ForEachQueuedSong(Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachLibrarySong(Object & object, void (Object::*callBack)(Mpc::Song *));

   private:
      unsigned int QueueVersion();
      Song * CreateSong(uint32_t id, mpd_song const * const) const;

   private:
      void CheckError();
      void DeleteConnection();

   private:
      Main::Vimpc *           vimpc_;
      struct mpd_connection * connection_;

      struct mpd_song *       currentSong_;
      struct mpd_status *     currentStatus_;
      int32_t                 currentSongId_;
      std::string             currentSongURI_;
      std::string             currentState_;

      Ui::Screen      const & screen_;
      unsigned int            queueVersion_;
      bool                    forceUpdate_;
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

}

#endif
