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
#include <vector>

#include "assert.hpp"
#include "callback.hpp"
#include "window/window.hpp"

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
      virtual size_t Size() const = 0;
      virtual std::string String(uint32_t position) const { return ""; }
   };

   //
   //! \todo delete callbacks on destruction
   //! Window buffer
   template <typename T>
   class Buffer : public WindowBuffer, private std::vector<T>
   {
   private:
      typedef std::vector<Main::CallbackInterface<T> *> CallbackList;
      typedef std::map<BufferCallbackEvent, CallbackList> CallbackMap;

   public:
      typedef T BufferType;

   public:
      Buffer<T>() { }
      ~Buffer<T>()
      {
         for (typename CallbackMap::iterator it = callback_.begin(); (it != callback_.end()); ++it)
         {
            for (typename CallbackList::const_iterator jt = it->second.begin(); (jt != it->second.end()); ++jt)
            {
               delete *jt;
            }
         }
      }

   private:
      Buffer<T>(Buffer<T> const & buffer);
      Buffer<T> & operator=(Buffer<T> const & buffer);

   public:
      T const & Get(uint32_t position) const
      {
         return Buffer<T>::at(position);
      }

      void Add(T entry)
      {
         Buffer<T>::push_back(entry);
         Callback(Buffer_Add, entry);
      }

      void Replace(uint32_t index, T entry)
      {
         if (index < Size())
         {
            Callback(Buffer_Remove, Buffer<T>::at(index));
            Buffer<T>::at(index) = entry;
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

         typename Buffer<T>::const_iterator it;

         for (it = Buffer<T>::begin(); ((it != Buffer<T>::end()) && (*it != entry)); ++pos, ++it) { }

         if (it == Buffer<T>::end())
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
            typename Buffer<T>::iterator it;

            for (it = Buffer<T>::begin(); ((pos != position) && (it != Buffer<T>::end())); ++it, ++pos) { }

            Buffer<T>::insert(it, entry);

            Callback(Buffer_Add, entry);
         }
      }

      void Crop(uint32_t newSize)
      {
         while (newSize < Size())
         {
            T entry = Buffer<T>::back();
            Buffer<T>::pop_back();
            Callback(Buffer_Remove, entry);
         }
      }

      void ForEach(uint32_t position, uint32_t count, CallbackInterface<T> * callback) const
      {
         uint32_t pos = 0;
         typename Buffer<T>::const_iterator it;

         for (it = Buffer<T>::begin(); ((pos != position) && (it != Buffer<T>::end())); ++it, ++pos) { }

         for (uint32_t c = 0; ((c < count) && (it != Buffer<T>::end())); ++c, ++it)
         {
            (*callback)(*it);
         }
      }

      void Remove(uint32_t position, uint32_t count)
      {
         uint32_t pos = 0;
         typename Buffer<T>::iterator it;

         for (it = Buffer<T>::begin(); ((pos < position) && (it != Buffer<T>::end())); ++it, ++pos) { }

         for (uint32_t c = 0; ((c < count) && (it != Buffer<T>::end())); ++c)
         {
            T entry = *it;
            it = Buffer<T>::erase(it);
            Callback(Buffer_Remove, entry);
         }
      }

      template <class V>
      void Sort(V comparator)
      {
         std::sort(Buffer<T>::begin(), Buffer<T>::end(), comparator);
      }

      void Clear()
      {
         // We need to remove one by one to ensure
         // that the callback is called at the right time
         Remove(0, Size());
         ENSURE(Size() == 0);
      }

      size_t Size() const
      {
         return Buffer<T>::size();
      }

   public:
      void AddCallback(BufferCallbackEvent event, CallbackInterface<T> * callback)
      {
         callback_[event].push_back(callback);
      }

   private:
      void Callback(BufferCallbackEvent event, T & param) const
      {
         typename CallbackMap::const_iterator entry = callback_.find(event);

         if (entry != callback_.end())
         {
            for (typename CallbackList::const_iterator it = entry->second.begin(); (it != entry->second.end()); ++it)
            {
               (*(*it))(param);
            }
         }
      }

   private:
      CallbackMap callback_;
   };
}
#endif
/* vim: set sw=3 ts=3: */
