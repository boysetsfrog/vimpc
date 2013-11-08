/*
   Vimpc
   Copyright (C) 2010 - 2013 Nathan Sweetman

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

   compiler.hpp - platform settings and typedefs, etc
   */

#ifndef __MAIN__COMPILER
#define __MAIN__COMPILER

#ifdef USE_BOOST_FOREACH
#include <boost/foreach.hpp>
#endif

#ifdef USE_BOOST_THREAD
#include <boost/thread.hpp>
#else
#include <atomic>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <thread>
#endif

#ifdef USE_BOOST_FUNCTIONAL
#include <boost/functional.hpp>
#else
#include <functional>
#endif

#ifdef USE_BOOST_THREAD
typedef boost::thread             Thread;
typedef boost::mutex              Mutex;
typedef boost::recursive_mutex    RecursiveMutex;
typedef boost::condition_variable ConditionVariable;
#define Atomic(X) X
#define UniqueLock boost::unique_lock

template <typename T>
bool ConditionWait(ConditionVariable & Condition, UniqueLock<T> & Lock, int TimeoutMs)
{
   return Condition.timed_wait(Lock, boost::posix_time::milliseconds(TimeoutMs));
}
#else
typedef std::thread               Thread;
typedef std::mutex                Mutex;
typedef std::recursive_mutex      RecursiveMutex;
typedef std::condition_variable   ConditionVariable;
#define Atomic(X) std::atomic<X>
#define UniqueLock std::unique_lock

template <typename T>
bool ConditionWait(ConditionVariable & Condition, UniqueLock<T> & Lock, int TimeoutMs)
{
   return (Condition.wait_for(Lock, std::chrono::milliseconds(TimeoutMs)) != std::cv_status::timeout);
}
#endif

#ifdef USE_BOOST_FUNCTIONAL
#define FUNCTION boost::function
#else
#define FUNCTION std::function
#endif

#ifdef USE_BOOST_FOREACH
#define FOREACH BOOST_FOREACH
#else
#define FOREACH(X, Y) for (X : Y)
#endif


#endif

/* vim: set sw=3 ts=3: */
