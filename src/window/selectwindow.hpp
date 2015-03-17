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

   selectwindow.hpp - window that is scrollable and has selectable elements
   */

#ifndef __UI__SELECTWINDOW
#define __UI__SELECTWINDOW

#include "song.hpp"
#include "scrollwindow.hpp"

namespace Main { class Settings; }

namespace Ui
{
   typedef std::pair<int64_t, int64_t> Selection;

   class SelectWindow : public Ui::ScrollWindow
   {
   public:
      SelectWindow(Main::Settings const & settings, Ui::Screen & screen, std::string name = "Unknown");
      ~SelectWindow();

   protected:
      void Print(uint32_t line) const;
      void Resize(uint32_t rows, uint32_t columns);

   public:
      void Scroll(int32_t scrollCount);
      void ScrollTo(uint32_t scrollLine);

      uint32_t CurrentLine() const;
      void Confirm();

   public: // Ui::ScrollWindow
      void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void Escape();
      void Visual();
      void SwitchVisualEnd();
      bool InVisualMode();
      void SetSupportsVisual(bool enable) { supportsVisual_ = enable; }

   public:
      bool IsSelected(uint32_t line) const;
      Selection CurrentSelection() const;
      void ResetSelection();

   protected:
      virtual void LimitCurrentSelection();

   private:
      void UpdateLastSelection();

   protected:
      int64_t           currentLine_;

   private:
      bool              visualMode_;
      bool              supportsVisual_;
      Selection         currentSelection_;
      Selection         lastSelection_;
      bool              hadSelection_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
