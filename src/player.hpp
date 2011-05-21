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
      bool ClearScreen();

      //! Connects the mpd client to the given host
      //!
      //! \param host The hostname to connect to
      //! \param port The port to connect with
      bool Connect(std::string const & host, uint32_t port = 0);

      //! Echos a string to the console window
      //!
      //! \param echo The string to be echoed 
      bool Echo(std::string const & echo);

      //! Pauses the current playback
      bool Pause();

      //! Plays the song with the given \p id
      bool Play(uint32_t position);

      //! Quits the program
      bool Quit();

      //! Toggles random on or off
      bool ToggleRandom();

      //! Toggles single on or off
      bool ToggleSingle();

      //! Disables/Enables random functionality
      bool SetRandom(bool random);

      //! Get the current random state
      bool Random();

      //! Disables/Enables single functionality
      bool SetSingle(bool single);

      //! Get the current single state
      bool Single();

      //! Redraws the current window
      bool Redraw();

      //! Stops playback
      bool Stop();

   public:
      //! Rescan the library
      bool Rescan();

      //! Update the library
      bool Update();

      typedef enum 
      { 
         Next,
         Previous
      } Skip;

      //! Skips forwards or backwards a given number of songs
      //!
      //! \param skip  Direction to skip in playlist
      //! \param count Number of songs to skip
      bool SkipSong(Skip skip, uint32_t count);

      bool SkipAlbum(Skip skip, uint32_t count);
      
      //! Skips forwards or backwards a given number of artists
      //!
      //! \param skip  Direction to skip in playlist
      //! \param count Number of artists to skip
      bool SkipArtist(Skip skip, uint32_t count);

   protected:
      //! Returns the currently playling song
      //!
      //! \return Id of currently playing song
      uint32_t GetCurrentSong() const;

   private:
      bool     SkipSongByInformation(Skip skip, uint32_t count, Mpc::Song::SongInformationFunction songFunction);
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
