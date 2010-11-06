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

namespace Ui
{
   class Screen;
}

namespace Mpc
{
   class Client
   {
   public:
      Client(Ui::Screen & screen);
      ~Client();

   public:
      void Start();

      void Connect();
      void Play(uint32_t playId);
      void Pause();
      void Next();
      void Previous();
      void Stop();
      void Random(bool randomOn);

   public:
      int32_t GetCurrentSong() const;
      int32_t TotalNumberOfSongs() const;

   public:
      void DisplaySongInformation();

   private:
      void CheckError();

   private:
      bool started_;

      Ui::Screen & screen_;

      struct mpd_connection * connection_;

   };
}

#endif
