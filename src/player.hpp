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
   class Playlist;
}

namespace Ui
{
   class Screen;
}

//! \todo why is this in the ui namespace?
namespace Ui
{
   // Class for functionality that is shared between all
   // modes and needs to be accessed via actions or commands, etc
   class Player
   {
   public:
      Player(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings);
      virtual ~Player() = 0;

   protected: //Commands which may be called by the mode
      //! Clears the current window
      void ClearScreen();

      //! Pauses the current playback
      void Pause();

      //! Plays the song with the given \p id
      void Play(uint32_t position);

      //! Seek within the current song
      void Seek(int32_t Offset);
      void SeekTo(uint32_t Time);

      //! Quits the program
      void Quit();

      //! Toggles consume on or off
      void ToggleConsume();

      //! Toggles repeat on or off
      void ToggleRepeat();

      //! Toggles random on or off
      void ToggleRandom();

      //! Toggles single on or off
      void ToggleSingle();

      //! Disables/Enables random functionality
      void SetRandom(bool random);

      //! Disables/Enables single functionality
      void SetSingle(bool single);

      //! Disables/Enables repeat functionality
      void SetRepeat(bool repeat);

      //! Disables/Enables consume functionality
      void SetConsume(bool consume);

      //! Shuffle the playlist
      void Shuffle();

      //! Redraws the current window
      void Redraw();

      //! Stops playback
      void Stop();

      //! Set the current volume
      void Volume(uint32_t volume);

      //! Load the playlist
      void LoadPlaylist(std::string const & name);

      //!
      void SavePlaylist(std::string const & name);

   public:
      //! Rescan the library
      void Rescan();

      //! Update the library
      void Update();

      typedef enum
      {
         Next,
         Previous
      } Skip;

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

   protected:
      //! Returns the currently playling song
      //!
      //! \return Id of currently playing song
      uint32_t GetCurrentSong() const;

   private:
      void     SkipSongByInformation(Skip skip, uint32_t count, Mpc::Song::SongInformationFunction songFunction);
      uint32_t NextSongByInformation(uint32_t startSong, Skip skip, Mpc::Song::SongInformationFunction songFunction);
      uint32_t First(Mpc::Song const * const song, uint32_t position, Mpc::Song::SongInformationFunction songFunction);

   private:
      //! Based upon the auto scroll setting, will determine whether to
      //! scroll the screen after a skip has been performed
      void HandleAutoScroll();

   private:
      Ui::Screen     & screen_;
      Mpc::Client    & client_;
      Mpc::Playlist  & playlist_;
      Main::Settings & settings_;
   };

}

#endif
