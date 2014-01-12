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

#ifndef __MAIN__BUFFER
#define __MAIN__BUFFER

#include <map>
#include <stdint.h>
#include <algorithm>
#include <vector>

#include "assert.hpp"
#include "compiler.hpp"
#include "window/window.hpp"

namespace Item
{
   typedef enum
   {
      Single,
      All
   } Collection;
}

namespace Main
{
   //! Events that will trigger a registered callback
   typedef enum
   {
      Buffer_Add,
      Buffer_Remove,
      Buffer_Replace
   } BufferCallbackEvent;


   class WindowBuffer
   {
   public:
      virtual ~WindowBuffer() { }
      virtual size_t Size() const = 0;
      virtual std::string String(uint32_t position) const { return ""; }
      virtual std::string PrintString(uint32_t position) const { return ""; }
   };

   //! Window buffer
   template <typename T>
   class BufferImpl : public WindowBuffer, private std::vector<T>
   {
   private:
      typedef FUNCTION<void (T)>         CallbackFunction;
      typedef std::vector<CallbackFunction>   CallbackList;
      typedef std::map<BufferCallbackEvent, CallbackList> CallbackMap;

   public:
      typedef T BufferType;

   public:
      BufferImpl<T>() { }
      virtual ~BufferImpl<T>() { }

   private:
      BufferImpl<T>(BufferImpl<T> const & buffer);
      BufferImpl<T> & operator=(BufferImpl<T> const & buffer);

   public:
      T const & Get(uint32_t position) const
      {
         T const & Result = BufferImpl<T>::at(position);
         return Result;
      }

      void Add(T entry)
      {
         BufferImpl<T>::push_back(entry);
         Callback(Buffer_Add, entry);
      }

      void Replace(uint32_t index, T entry)
      {
         if (index < Size())
         {
            Callback(Buffer_Replace, BufferImpl<T>::at(index));
            BufferImpl<T>::at(index) = entry;
            Callback(Buffer_Add, entry);
         }
         else
         {
            Add(entry);
         }
      }

      int32_t Index(T entry) const
      {
         int32_t pos = 0;

         typename BufferImpl<T>::const_iterator it;

         for (it = BufferImpl<T>::begin(); ((it != BufferImpl<T>::end()) && (*it != entry)); ++pos, ++it) { }

         if (it == BufferImpl<T>::end())
         {
            pos = -1;
         }

         return pos;
      }

      void AddFront(T entry)
      {
         Add(entry, 0);
      }

      void Add(T entry, uint32_t position)
      {
         if (position <= Size())
         {
            uint32_t pos = 0;
            typename BufferImpl<T>::iterator it;

            for (it = BufferImpl<T>::begin(); ((pos != position) && (it != BufferImpl<T>::end())); ++it, ++pos) { }

            BufferImpl<T>::insert(it, entry);

            Callback(Buffer_Add, entry);
         }
      }

      void Crop(uint32_t newSize)
      {
         while (newSize < Size())
         {
            T entry = BufferImpl<T>::back();
            BufferImpl<T>::pop_back();
            Callback(Buffer_Remove, entry);
         }
      }

      void ForEach(uint32_t position, uint32_t count, FUNCTION<void (T)> callback) const
      {
         uint32_t pos = 0;
         typename BufferImpl<T>::const_iterator it;

         for (it = BufferImpl<T>::begin(); ((pos != position) && (it != BufferImpl<T>::end())); ++it, ++pos) { }

         for (uint32_t c = 0; ((c < count) && (it != BufferImpl<T>::end())); ++c, ++it)
         {
            (*callback)(*it);
         }
      }

      void Remove(uint32_t position, uint32_t count)
      {
         uint32_t pos = 0;
         typename BufferImpl<T>::iterator it;

         for (it = BufferImpl<T>::begin(); ((pos < position) && (it != BufferImpl<T>::end())); ++it, ++pos) { }

         for (uint32_t c = 0; ((c < count) && (it != BufferImpl<T>::end())); ++c)
         {
            T entry = *it;
            it = BufferImpl<T>::erase(it);
            Callback(Buffer_Remove, entry);
         }
      }

      template <class V>
      void Sort(V comparator)
      {
         std::sort(BufferImpl<T>::begin(), BufferImpl<T>::end(), comparator);
      }

      void Clear()
      {
         // We need to remove one by one to ensure
         // that the callback is called at the right time
         for (auto it = BufferImpl<T>::begin(); (it != BufferImpl<T>::end()); ++it)
         {
            Callback(Buffer_Remove, *it);
         }

         BufferImpl<T>::clear();

         ENSURE(Size() == 0);
      }

      size_t Size() const
      {
         return BufferImpl<T>::size();
      }

   public:
      void AddCallback(BufferCallbackEvent event, CallbackFunction callback)
      {
         callback_[event].push_back(callback);
      }

   private:
      void Callback(BufferCallbackEvent event, T & param) const
      {
         auto const it = callback_.find(event);

         if (it != callback_.end())
         {
            FOREACH(auto func, it->second)
            {
               (func)(param);
            }
         }
      }

   private:
      CallbackMap callback_;
   };

   template <typename T>
   class Buffer : public BufferImpl<T> { };

   template <>
   class Buffer<std::string> : public BufferImpl<std::string>
   {
   public:
      std::string PrintString(uint32_t position) const { return Get(position); }
      std::string String(uint32_t position) const { return Get(position); }
   };
}


#endif
/* vim: set sw=3 ts=3: */
