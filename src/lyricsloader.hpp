/*
   Vimpc
   Copyright (C) 2010 - 2016 Nathan Sweetman

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

   lyricsloader.hpp - threaded/automated lyrics loading
   */

#ifndef __MAIN__LYRICSLOADER
#define __MAIN__LYRICSLOADER

#include <string>
#include <vector>

#include "buffers.hpp"
#include "compiler.hpp"
#include "events.hpp"
#include "song.hpp"
#include "lyricsfetcher.hpp"
#include "buffer/buffer.hpp"

namespace Main
{
   class LyricsLoader
   {
      public:
         static LyricsLoader & Instance();

      protected:
         LyricsLoader();
         ~LyricsLoader();

      public:
         void Load(Mpc::Song * song);

      private:
         void SongChanged(EventData const & Data);
         void ElapsedUpdate(uint32_t elapsed);
         void Load(std::string artist, std::string title, std::string uri, uint32_t duration);

      public:
         std::string  Artist()    { return artist_; }
         std::string  Title()     { return title_; }
         std::string  URI()       { return uri_; }
         bool         Loaded()    { return loaded_; }
         bool         IsLoading() { return loading_; }

      private:
         void LyricsQueueExecutor(Main::LyricsLoader * loader);

      private:
         bool           loaded_;
         bool           loading_;
         uint32_t       percent_;
         uint32_t       duration_;
         std::string    artist_;
         std::string    title_;
         std::string    uri_;
         Main::Lyrics & lyrics_;
         Thread         lyricsThread_;
   };

}

#endif
