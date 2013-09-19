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
#include <mutex>
#include <stdint.h>
#include <vector>

#include "assert.hpp"
#include "callback.hpp"
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
      Buffer_Remove
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
      typedef std::vector<Main::CallbackInterface<T> *> CallbackList;
      typedef std::map<BufferCallbackEvent, CallbackList> CallbackMap;

   public:
      typedef T BufferType;

   public:
      BufferImpl<T>() { }
      ~BufferImpl<T>()
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);

         for (typename CallbackMap::iterator it = callback_.begin(); (it != callback_.end()); ++it)
         {
            for (typename CallbackList::const_iterator jt = it->second.begin(); (jt != it->second.end()); ++jt)
            {
               delete *jt;
            }
         }
      }

   private:
      BufferImpl<T>(BufferImpl<T> const & buffer);
      BufferImpl<T> & operator=(BufferImpl<T> const & buffer);

   public:
      T const & Get(uint32_t position) const
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);
         T const & Result = BufferImpl<T>::at(position);
         return Result;
      }

      void Add(T entry)
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);
         BufferImpl<T>::push_back(entry);
         Callback(Buffer_Add, entry);
      }

      void Replace(uint32_t index, T entry)
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);

         if (index < Size())
         {
            Callback(Buffer_Remove, BufferImpl<T>::at(index));
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
         std::unique_lock<std::recursive_mutex> lock(mutex_);
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
         std::unique_lock<std::recursive_mutex> lock(mutex_);

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
         std::unique_lock<std::recursive_mutex> lock(mutex_);

         while (newSize < Size())
         {
            T entry = BufferImpl<T>::back();
            BufferImpl<T>::pop_back();
            Callback(Buffer_Remove, entry);
         }
      }

      void ForEach(uint32_t position, uint32_t count, CallbackInterface<T> * callback) const
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);

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
         std::unique_lock<std::recursive_mutex> lock(mutex_);

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
         std::unique_lock<std::recursive_mutex> lock(mutex_);
         std::sort(BufferImpl<T>::begin(), BufferImpl<T>::end(), comparator);
      }

      void Clear()
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);

         // We need to remove one by one to ensure
         // that the callback is called at the right time
         Remove(0, Size());
         ENSURE(Size() == 0);
      }

      size_t Size() const
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);
         return BufferImpl<T>::size();
      }

   public:
      void AddCallback(BufferCallbackEvent event, CallbackInterface<T> * callback)
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);
         callback_[event].push_back(callback);
      }

   private:
      void Callback(BufferCallbackEvent event, T & param) const
      {
         std::unique_lock<std::recursive_mutex> lock(mutex_);
         typename CallbackMap::const_iterator entry = callback_.find(event);

         if (entry != callback_.end())
         {
            for (typename CallbackList::const_iterator it = entry->second.begin(); (it != entry->second.end()); ++it)
            {
               (*(*it))(param);
            }
         }
      }

   protected:
      mutable std::recursive_mutex	mutex_;

   private:
      CallbackMap                   callback_;
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
