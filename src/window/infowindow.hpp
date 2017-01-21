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

   infowindow.hpp - class representing a window that can scroll
   */

#ifndef __UI__INFOWINDOW
#define __UI__INFOWINDOW

#include "song.hpp"
#include "songwindow.hpp"

namespace Ui
{
   class Screen;

   class InfoWindow : public SongWindow
   {
   public:
      InfoWindow(std::string const & URI, Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search, std::string name = "Unknown");
      virtual ~InfoWindow();

   private:
      void Print(uint32_t line) const;

   public:
      uint32_t Current() const              { return CurrentLine(); };
      std::string SearchPattern(uint32_t id) const { return ""; }

   public:
      void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void Edit();
      void Redraw();

      void AddAllLines() {}
      void CropLine(uint32_t line, uint32_t count = 1, bool scroll = true) {}
      void CropAllLines() {}
      void DeleteAllLines() {}
      void Save(std::string const & name);

   private:
      std::string m_URI;
   };
}

#endif
/* vim: set sw=3 ts=3: */
