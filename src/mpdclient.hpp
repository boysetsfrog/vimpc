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

#include "song.hpp"

namespace Ui
{
   class Screen;
}

// \todo cache all the values that we can
namespace Mpc
{
   class Client
   {
   public:
      Client(Ui::Screen const & screen);
      ~Client();

   private:
      Client(Client & client);
      Client & operator=(Client & client);

   public:
      void Connect(std::string const & hostname = "");
      void Play(uint32_t playId);
      void Pause();
      void Next();
      void Previous();
      void Stop();
      void Random(bool randomOn);

   public: //Database
      void Rescan();
      void Update();

   public:
      std::string CurrentState();

   public:
      bool Connected() const;

   public:
      int32_t GetCurrentSongId() const;
      uint32_t TotalNumberOfSongs();

   public:
      void DisplaySongInformation();

   public:
      template <typename Object>
      void ForEachQueuedSong(Object & object, void (Object::*callBack)(Song const * const));

      template <typename Object>
      void ForEachLibrarySong(Object & object, void (Object::*callBack)(Song const * const));

   private:
      Song * const CreateSong(mpd_song const * const) const;

   private:
      uint32_t SecondsToMinutes(uint32_t duration) const;
      uint32_t RemainingSeconds(uint32_t duration) const;

   private:
      void CheckError();

   private:
      struct mpd_connection * connection_;
      bool mutable currentSongHasChanged_;
      Ui::Screen      const & screen_;
   };

   //
   template <typename Object>
   void Client::ForEachQueuedSong(Object & object, void (Object::*callBack)(Song const * const))
   {
      if (Connected() == true)
      {
         mpd_send_list_queue_meta(connection_);

         mpd_song * nextSong = mpd_recv_song(connection_);

         for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
         {
            Song const * const newSong = CreateSong(nextSong);

            (object.*callBack)(newSong);

            mpd_song_free(nextSong);
            delete newSong;
         }
      }
   }

   //
   template <typename Object>
   void Client::ForEachLibrarySong(Object & object, void (Object::*callBack)(Song const * const))
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
                  Song const * const newSong = CreateSong(nextSong);

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
