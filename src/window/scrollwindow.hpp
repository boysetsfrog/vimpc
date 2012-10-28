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

   class ScrollWindow : public Window
   {
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

   public:
      virtual void Print(uint32_t line) const;
      virtual void Resize(int rows, int columns);
      virtual void Scroll(int32_t scrollCount);
      virtual void ScrollTo(uint16_t scrollLine);
      virtual void ScrollToFirstMatch(std::string const & input) { }
      virtual void ScrollToCurrent() { }
      virtual void ResetSelection() {}

      virtual uint32_t Current() const           { return CurrentLine(); };
      virtual uint32_t Playlist(int count) const { return Current(); };
      virtual std::string SearchPattern(int32_t id) const { return ""; }

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
      virtual void Save(std::string const & name) {}

   public:
      size_t BufferSize() const { return WindowBuffer().Size(); }
      int32_t ContentSize() const { return WindowBuffer().Size() - 1; }

   public:
      std::string const & Name();
      void SetName(std::string const &);

      bool Select(Position position, uint32_t count);
      void SetAutoScroll(bool autoScroll);
      bool AutoScroll() const;

   public:
      uint32_t FirstLine()   const;
      uint32_t LastLine()    const { return (BufferSize() < ScrollLine()) ? BufferSize() : ScrollLine(); }
      virtual  uint16_t CurrentLine() const { return FirstLine(); }

   protected:
      void ResetScroll();
      void SetScrollLine(uint16_t scrollLine);
      uint16_t ScrollLine() const;

   protected:
      void SoftRedrawOnSetting(Setting::ToggleSettings setting);
      void SoftRedrawOnSetting(Setting::StringSettings setting);
      void OnSettingChanged(bool)        { SoftRedraw(); }
      void OnSettingChanged(std::string) { SoftRedraw(); }

   protected:
      virtual void SoftRedraw() {}
      virtual Main::WindowBuffer const & WindowBuffer() const = 0;

   protected:
      Ui::Screen &               screen_;
      std::string                name_;
      uint16_t                   scrollLine_;
      bool                       autoScroll_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
