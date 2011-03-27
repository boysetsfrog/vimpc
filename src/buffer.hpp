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
#include "callback.hpp"
#include "window.hpp"

namespace Main
{
   //! Events that will cause a callback
   typedef enum
   {
      Buffer_Add,
      Buffer_Remove
   } BufferCallbackEvent;

   //! Window buffer
   template <typename T>
   class Buffer : private std::vector<T>
   {
   private:
      typedef std::vector<T> BufferType;
      typedef std::vector<boost::function<void (T &)> > CallbackList;
      typedef std::map<BufferCallbackEvent, CallbackList> CallbackMap;

   public:
      typedef T BufferParameter;

   public:
      template <class H, class I>
      void AddCallback(BufferCallbackEvent event, CallbackObject<H, I> * callback)
      { 
         callback_[event].push_back(*callback);
      }

   public:
      virtual T const & Get(uint32_t position) const
      { 
         return Buffer<T>::at(position);
      }

      virtual void Add(T entry)
      { 
         Buffer<T>::push_back(entry); 

         Callback(Buffer_Add, entry);
      }

      virtual void Add(T entry, uint32_t position)
      { 
         if (position <= Size())
         {
            uint32_t pos = 0;
            typename Buffer<T>::iterator it;

            for (it = std::vector<T>::begin(); ((pos != position) && (it != std::vector<T>::end())); ++it, ++pos) { }

            std::vector<T>::insert(it, entry);

            Callback(Buffer_Add, entry);
         }
      }

      virtual void Remove(uint32_t position, uint32_t count)
      { 
         uint32_t pos = 0;
         typename Buffer<T>::iterator it;

         for (it = Buffer<T>::begin(); ((pos != position) && (it != Buffer<T>::end())); ++it, ++pos) { }

         for (uint32_t c = 0; ((c < count) && (it != Buffer<T>::end())); ++c)
         {
            Callback(Buffer_Remove, *it);
            it = std::vector<T>::erase(it);
         }
      } 

      virtual void Clear()
      { 
         for (typename Buffer<T>::iterator it = Buffer<T>::begin(); (it != Buffer<T>::end()); ++it)
         {
            Callback(Buffer_Remove, *it);
         }

         Buffer<T>::clear(); 
      }

      virtual size_t Size() const                   
      { 
         return std::vector<T>::size(); 
      } 

   private:
      void Callback(BufferCallbackEvent event, T & param)
      {
         for (typename CallbackList::iterator it = callback_[event].begin(); (it != callback_[event].end()); ++it)
         {
            (*it)(param);
         }
      }
      
   private:
      CallbackMap callback_;
   };
}
#endif
