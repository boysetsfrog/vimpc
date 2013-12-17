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

   player.hpp - manages the use of the mpdclient and screen for
   common functionality between modes.
   */

#ifndef __UI__PLAYER
#define __UI__PLAYER

#include <string>
#include <stdint.h>

#include "output.hpp"
#include "mpdclient.hpp"
#include "screen.hpp"
#include "song.hpp"

namespace Main
{
   class Settings;
}

namespace Mpc
{
   class Client;
   class ClientState;
   class Playlist;
}

namespace Ui
{
   class Screen;
}

namespace Ui
{
   // Class for functionality that is shared between all
   // modes and needs to be accessed via actions or commands, etc
   class Player
   {
   public:
      Player(Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Main::Settings & settings);
      virtual ~Player() = 0;

   protected: //Commands which may be called by the mode
      void ClearScreen();

      void Pause();

      //! Plays the song with the given \p id
      void Play(uint32_t position);

      //! Seek within the current song
      void Seek(int32_t Offset);
      void SeekTo(uint32_t Time);

      //! Quits the program
      void Quit();

      void ToggleConsume();
      void ToggleCrossfade();
      void ToggleRepeat();
      void ToggleRandom();
      void ToggleSingle();

      void SetRandom(bool random);
      void SetSingle(bool single);
      void SetRepeat(bool repeat);
      void SetConsume(bool consume);

      void SetCrossfade(bool crossfade);
      void SetCrossfade(uint32_t crossfade);

      int32_t FindOutput(std::string const & outputName);

      void SetOutput(Item::Collection collection, bool enable);
      void SetOutput(uint32_t output, bool enable);
      void ToggleOutput(Item::Collection collection);
      void ToggleOutput(uint32_t output);

      void Shuffle();

      void Redraw();

      void Stop();
      void Volume(uint32_t volume);

      void LoadPlaylist(std::string const & name);
      void SavePlaylist(std::string const & name);

   public:
      void Rescan();
      void Update();

      typedef enum
      {
         Next,
         Previous
      } Skip;

      typedef enum
      {
         Start,
         End
      } Location;

      //! Skips forwards or backwards a given number of songs
      //!
      //! \param skip  Direction to skip in playlist
      //! \param count Number of songs to skip
      void SkipSong(Skip skip, uint32_t count);

      void SkipAlbum(Skip skip, uint32_t count);

      //! Skips forwards or backwards a given number of artists
      //!
      //! \param skip  Direction to skip in playlist
      //! \param count Number of artists to skip
      void SkipArtist(Skip skip, uint32_t count);

   public:
      //! Based upon the auto scroll setting, will determine whether to
      //! scroll the screen after a skip has been performed
      void HandleAutoScroll();

   protected:
      //! Returns the currently playing song's id
      //!
      //! \return Id of currently playing song
      int32_t GetCurrentSongPos() const;

   private:
      void     SkipSongByInformation(Skip skip, uint32_t count, Mpc::Song::SongInformationFunction songFunction);
      uint32_t NextSongByInformation(uint32_t startSong, Skip skip, Mpc::Song::SongInformationFunction songFunction);
      uint32_t First(Mpc::Song const * const song, uint32_t position, Mpc::Song::SongInformationFunction songFunction);

   private:
      Ui::Screen &        screen_;
      Mpc::Client &       client_;
      Mpc::ClientState &  clientState_;
      Mpc::Playlist  &    playlist_;
      Main::Settings &    settings_;
   };

}

#endif
/* vim: set sw=3 ts=3: */
