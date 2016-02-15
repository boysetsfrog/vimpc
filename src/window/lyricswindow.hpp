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

   lyricswindow.hpp - class representing a window that can scroll
   */

#ifndef __UI__LYRICSWINDOW
#define __UI__LYRICSWINDOW

#include <vector>

#include "buffers.hpp"
#include "lyricsfetcher.hpp"
#include "song.hpp"

#include "buffer/buffer.hpp"
#include "window/selectwindow.hpp"

namespace Main { class Settings; }
namespace Ui   { class Search; }

namespace Ui
{
   class LyricsWindow : public Ui::SelectWindow
   {
   public:
      LyricsWindow(std::string const & URI, Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search, std::string name = "Unknown");
      ~LyricsWindow();

   private:
      void Print(uint32_t line) const;

   public:
      void Redraw();
      void Edit();
      void Scroll(int32_t scrollCount);
      void ScrollTo(uint32_t scrollLine);

      std::string SearchPattern(uint32_t id) const { return lyrics_.Get(id); }

   protected:
      Main::WindowBuffer const & WindowBuffer() const { return lyrics_; }

   private:
      void Clear();
      void LoadLyrics();
      void LyricsLoaded();

   private:
      std::string 			  m_URI;
      Main::Settings const & settings_;
      Ui::Search     const & search_;
      Main::Lyrics 	        lyrics_;
   };
}

#endif // __UI__LYRICSWINDOW

/* vim: set sw=3 ts=3: */
