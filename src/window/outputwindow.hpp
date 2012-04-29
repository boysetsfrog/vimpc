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

   outputwindow.hpp - selection of the output to use
   */

#ifndef __UI__OUTPUTWINDOW
#define __UI__OUTPUTWINDOW

// Includes
#include <iostream>

#include "buffer/outputs.hpp"
#include "window/selectwindow.hpp"

// Forward Declarations
namespace Main { class Settings; }
namespace Mpc  { class Client; class Output; }
namespace Ui   { class Search; }

// Output window class
namespace Ui
{
   class OutputComparator
   {
      public:
      bool operator() (Mpc::Output* i, Mpc::Output* j) { return (*i<*j); };
   };

   class OutputWindow : public Ui::SelectWindow
   {
   public:
      OutputWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Ui::Search const & search);
      ~OutputWindow();

   private:
      OutputWindow(OutputWindow & window);
      OutputWindow & operator=(OutputWindow & window);

   public:
      void Print(uint32_t line) const;
      void Left(Ui::Player & player, uint32_t count);
      void Right(Ui::Player & player, uint32_t count);
      void Confirm();
      void Redraw();

      uint32_t Current() const { return CurrentLine(); }

   public:
      void AdjustScroll(Mpc::Output * output);

   public:
      std::string SearchPattern(int32_t id) const { return outputs_.Get(id)->Name(); }

   public:
      void AddLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void AddAllLines();
      void DeleteLine(uint32_t line, uint32_t count = 1, bool scroll = true);
      void DeleteAllLines();

   private:
      void SetOutput(uint32_t line, bool enable, uint32_t count = 1, bool scroll = true);

   private:
      void    Clear();
      size_t  BufferSize() const { return outputs_.Size(); }
      int32_t DetermineColour(uint32_t line) const;

   private:
      Main::Settings const & settings_;
      Mpc::Client          & client_;
      Ui::Search     const & search_;
      Mpc::Outputs         & outputs_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
