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

   song.hpp - represents information of an individual track
   */

#include "song.hpp"

#include <stdio.h>

#include "buffer/library.hpp"

const std::string UnknownArtist = "Unknown Artist";
const std::string UnknownAlbum  = "Unknown Album";
const std::string UnknownTitle  = "Unknown";
const std::string UnknownURI    = "Unknown";

using namespace Mpc;

Song::Song() :
   reference_ (0),
   artist_    (""),
   album_     (""),
   title_     (""),
   track_     (""),
   uri_       (""),
   duration_  (0),
   lastFormat_(""),
   formatted_ (""),
   entry_     (NULL)
{ }

Song::Song(Song const & song) :
   reference_ (0),
   artist_    (song.Artist()),
   album_     (song.Album()),
   title_     (song.Title()),
   track_     (song.Track()),
   uri_       (song.URI()),
   duration_  (song.Duration()),
   lastFormat_(song.lastFormat_),
   formatted_ (song.formatted_)
{
   SetDuration(duration_);
}

Song::~Song()
{ }


int32_t Song::Reference() const
{
   return reference_;
}

/* static */ void Song::IncrementReference(Song * song)
{
   song->reference_ += 1;

   if ((song->entry_ != NULL) && (song->reference_ == 1))
   {
      song->entry_->AddedToPlaylist();
   }
}

/* static */ void Song::DecrementReference(Song * song)
{
   song->reference_ -= 1;

   if ((song->entry_ != NULL) && (song->reference_ == 0))
   {
      song->entry_->RemovedFromPlaylist();
   }
}

/* static */ void Song::SwapThe(std::string & String)
{
   if (pcrecpp::RE("^\\s*[tT][hH][eE]\\s+").Replace("", &String))
   {
      String += ", The";
   }
}

void Song::SetArtist(const char * artist)
{
   if (artist != NULL)
   {
      artist_ = artist;
   }
   else
   {
      artist_ = UnknownArtist;
   }
}

std::string const & Song::Artist() const
{
   return artist_;
}

void Song::SetAlbum(const char * album)
{
   if (album != NULL)
   {
      album_ = album;
   }
   else
   {
      album_ = UnknownAlbum;
   }
}

std::string const & Song::Album() const
{
   return album_;
}

void Song::SetTitle(const char * title)
{
   if (title != NULL)
   {
      title_ = title;
   }
   else
   {
      title_ = UnknownTitle;
   }
}

std::string const & Song::Title() const
{
   return title_;
}

void Song::SetTrack(const char * track)
{
   if (track != NULL)
   {
      track_ = track;
   }
   else
   {
      track = "";
   }
}

std::string const & Song::Track() const
{
   return track_;
}

void Song::SetURI(const char * uri)
{
   if (uri != NULL)
   {
      uri_ = uri;
   }
   else
   {
      uri_ = UnknownURI;
   }
}

std::string const & Song::URI() const
{
   return uri_;
}

void Song::SetDuration(int32_t duration)
{
   duration_ = duration;

   char cduration[32];
   uint32_t const minutes = static_cast<uint32_t>(duration_ / 60);
   uint32_t const seconds = (duration_ - (minutes * 60));

   snprintf(cduration, 32, "%2d:%.2d", minutes, seconds);
   durationString_ = (std::string(cduration));
}

int32_t Song::Duration() const
{
   return duration_;
}

void Song::SetEntry(LibraryEntry * entry)
{
   entry_ = entry;
}

LibraryEntry * Song::Entry() const
{
   return entry_;
}

std::string const & Song::DurationString() const
{
   return durationString_;
}


std::string Song::FormatString(std::string fmt) const
{
   typedef std::string const & (Mpc::Song::*SongFunction)() const;
   static std::map<char, SongFunction> songInfo;

   if (songInfo.size() == 0)
   {
      songInfo['a'] = &Mpc::Song::Artist;
      songInfo['A'] = &Mpc::Song::Artist;
      songInfo['b'] = &Mpc::Song::Album;
      songInfo['B'] = &Mpc::Song::Album;
      songInfo['l'] = &Mpc::Song::DurationString;
      songInfo['t'] = &Mpc::Song::Title;
      songInfo['n'] = &Mpc::Song::Track;
      songInfo['f'] = &Mpc::Song::URI;
   }

   std::string result = fmt;

   if (lastFormat_ == fmt)
   {
      return formatted_;
   }

   lastFormat_ = fmt;

   int  j     = 0;
   int  right = -1;
   int  start = -1;
   bool valid = true;
   bool skip  = false;

   //! \TODO this really needs cleaning up, it is a disgrace

   for (int32_t i = 0; i < static_cast<int32_t>(fmt.size()); )
   {
      switch (fmt[i])
      {
         case '{':
         {
            start = j;
            valid = true;
            ++i;
            result.erase(j, 1);
            break;
         }
         case '}':
         {
            if (valid == false)
            {
               result.erase(start, j - start);
               j = start;
            }
            result.erase(j, 1);
            ++i;
            start = -1;
            break;
         }
         case '|':
         {
            if (valid == true)
            {
               skip = true; 
            }
            result.erase(j, 1);
            ++i;
            break;
         }
         case '%':
         {
            if (start == -1)
            {
               skip = false;
            }

            if ((i + 1) < static_cast<int32_t>(fmt.size()))
            {
               std::string next = "";

               if ((songInfo.find(fmt[i + 1]) != songInfo.end()) &&
                   (skip == false))
               {
                  SongFunction Function = songInfo[fmt[i + 1]];
                  next = (*this.*Function)();

                  if ((fmt[i + 1] == 'B') || (fmt[i + 1] == 'A'))
                  {
                     SwapThe(next);
                  }
               }

               if ((next == "") || (next.substr(0, strlen("Unknown")) == "Unknown"))
               {
                  valid = false;
               }

               result.replace(j, 2, next);
               j += next.size();
               i += 2;
            }
            else
            {
               ++i, ++j;
            }
            break;
         }
         default:
            ++i, ++j;
            break;
      }
   }

   formatted_ = result;
   return result;
}

/* vim: set sw=3 ts=3: */
