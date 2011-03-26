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

   buffer.hpp - 
   */

#ifndef __UI__BUFFER
#define __UI__BUFFER

#include <map>
#include <stdint.h>
#include <vector>

#include "attributes.hpp"
#include "window.hpp"

namespace Main
{
   namespace BufferState
   {
      typedef enum
      {
         Entry_Add,
         Entry_Remove
      } State;
   }      

   template <class T>
   class BufferInterface
   {
   public:
      virtual T const & Get(uint32_t position) const = 0;
      virtual void      Add(T entry) = 0; 
      virtual void      Add(T entry, uint32_t position) = 0;
      virtual void      Remove(T entry, uint32_t count) = 0;
      virtual void      Clear() = 0;
      virtual size_t    Size()  const = 0;
   };


   template <class T>
   class Buffer : public BufferInterface<T>, private std::vector<T>
   {
   private:
      typedef void (Ui::Window::*CallbackFunction)();
      typedef std::pair<Ui::Window *, CallbackFunction> CallbackPair;
      typedef std::vector<CallbackPair> CallbackList;
      typedef std::map<BufferState::State, CallbackList> CallbackMap;

   public:
      void AddCallback(BufferState::State state, Ui::Window * window, CallbackFunction callback) { CallbackPair tmp(window, callback); callback_[state].push_back(tmp); }

   public:
      virtual T const & Get(uint32_t position) const    { return std::vector<T>::at(position); }
      virtual void      Add(T entry)                    { std::vector<T>::push_back(entry); Callback(BufferState::Entry_Add); }
      virtual void      Add(UNUSED T entry, UNUSED uint32_t position) { Callback(BufferState::Entry_Add); }
      virtual void      Remove(UNUSED T entry, UNUSED uint32_t count) { Callback(BufferState::Entry_Remove); } 
      virtual void      Clear()                         { std::vector<T>::clear(); }
      virtual size_t    Size()  const                   { return std::vector<T>::size(); } 

   private:
      void Callback(BufferState::State state)
      {
         for (CallbackList::iterator it = callback_[state].begin(); (it != callback_[state].end()); ++it)
         {
            Ui::Window * const window = (*it).first;
            CallbackFunction function = (*it).second;

            (window->*function)();
         }
      }
      
   private:
      CallbackMap callback_;
   };
}
#endif
