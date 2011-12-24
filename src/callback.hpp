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

   callback.hpp - 
   */

#ifndef __MAIN__CALLBACK
#define __MAIN__CALLBACK

#include <algorithm>
#include <stdint.h>

#include "window/window.hpp"

namespace Main
{
   //! Class that provides an interface so that we can provide
   //! a function callback wrapper and a object/function wrapper
   template <class Parameter>
   class CallbackInterface
   {
      public:
      virtual void operator() (Parameter) = 0;
   };

   //! Object to wrap a callback function with an assosciated object
   template <class Object, class Parameter>
   class CallbackObject : public CallbackInterface<Parameter>
   {
      private: 
         typedef void (Object::*Callback)(Parameter);

      public:
         CallbackObject(Object & callee, Callback callback) :
            callee_   (callee),
            callback_ (callback)
         { }

         void operator() (Parameter param) { (callee_.*callback_)(param); }

      private:
         Object & callee_;
         Callback callback_;
   };

   //! Object to wrap a callback function
   template <class Parameter>
   class CallbackFunction : public CallbackInterface<Parameter>
   {
      private: 
         typedef void (*Callback)(Parameter);

      public:
         CallbackFunction(Callback callback) :
            callback_ (callback)
         { }

         void operator() (Parameter param) { (*callback_)(param); }

      private:
         Callback callback_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
