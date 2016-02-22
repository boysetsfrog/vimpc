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

   scrollwindow.hpp - class representing a window that can scroll
   */

#ifndef __UI__SCROLLWINDOW
#define __UI__SCROLLWINDOW

#include "buffer/buffer.hpp"
#include "settings.hpp"
#include "window.hpp"

namespace Ui
{
   class Screen;

   class ScrollWindow
   {
      friend class Ui::Screen;

   public:
      ScrollWindow(Ui::Screen & screen, std::string name = "Unknown");
      virtual ~ScrollWindow();

   public:
      typedef enum
      {
         First,
         Middle,
         Last,
         PositionCount
      } Position;

   protected:
      virtual void Print(uint32_t line) const;
      virtual void Resize(uint32_t rows, uint32_t columns);
      virtual void Redraw() {}

   public:
      virtual void Left(Ui::Player & player, uint32_t count) { }
      virtual void Right(Ui::Player & player, uint32_t count) { }
      virtual void Click() { }
      virtual void Confirm() { }

      virtual void Scroll(int32_t scrollCount);
      virtual void ScrollTo(uint32_t scrollLine);
      virtual void ScrollToFirstMatch(std::string const & input) { }
      virtual void ScrollToCurrent() { }
      virtual void ResetSelection() {}

      virtual uint32_t Current() const           { return CurrentLine(); };
      virtual uint32_t Playlist(int count) const { return Current(); };
      virtual std::string SearchPattern(uint32_t id) const { return ""; }

      bool IsEnabled() { return enabled_; }
      void Disable()   { enabled_ = false; }
      void Enable()    { enabled_ = true; }

   public:
      virtual void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true) {}
      virtual void AddAllLines() {}
      virtual void CropLine(uint32_t line, uint32_t count = 1, bool scroll = true) {}
      virtual void CropAllLines() {}
      virtual void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true) {}
      virtual void DeleteAllLines() {}
      virtual void Edit() {}
      virtual void Escape() {}
      virtual void Visual() {}
      virtual void SwitchVisualEnd() {}
#ifdef LYRICS_SUPPORT
      virtual void Lyrics() {}
#endif
      virtual void Save(std::string const & name) {}

   public:
      virtual uint32_t BufferSize() const { return WindowBuffer().Size(); }

   public:
      std::string const & Name() const;
      void SetName(std::string const &);

      bool Select(Position position, uint32_t count);
      void SetAutoScroll(bool autoScroll);
      bool AutoScroll() const;

   public:
      virtual bool IsSelected(uint32_t line) const { return false; }
      uint32_t FirstLine()   const;
      uint32_t LastLine()    const { return (BufferSize() < ScrollLine()) ? BufferSize() - 1 : ScrollLine() - 1; }
      virtual  uint32_t CurrentLine() const { return FirstLine(); }

      int32_t Rows() const { return rows_; }
      int32_t Columns() const { return cols_; }


   protected:
      WINDOW * N_WINDOW() const { return window_; }

   protected:
      void ResetScroll();

      uint32_t ScrollLine() const;
      void SetScrollLine(uint32_t scrollLine);

   protected:
      void SoftRedrawOnSetting(Setting::ToggleSettings setting);
      void SoftRedrawOnSetting(Setting::StringSettings setting);
      void OnSettingChanged(bool)        { SoftRedraw(); }
      void OnSettingChanged(std::string) { SoftRedraw(); }

   protected:
      virtual void SoftRedraw() {}
      virtual Main::WindowBuffer const & WindowBuffer() const = 0;
      virtual int32_t DetermineColour(uint32_t line) const;

   protected:
      Main::Settings &           settings_;
      Ui::Screen &               screen_;
      std::string                name_;
      WINDOW *                   window_;
      int32_t                    rows_;
      int32_t                    cols_;
      uint32_t                   scrollLine_;
      bool                       autoScroll_;
      bool                       enabled_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
