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

   song.hpp - represents information of an individual track
   */

#include "song.hpp"

#include <stdio.h>

using namespace Mpc;

Song::Song() :
   reference_(0),
   artist_   (""),
   album_    (""),
   title_    (""),
   uri_      (""),
   duration_ (0)
{ }

Song::Song(Song const & song) :
   reference_(0),
   artist_   (song.Artist()),
   album_    (song.Album()),
   title_    (song.Title()),
   uri_      (song.URI()),
   duration_ (song.Duration())
{ 
}

Song::~Song()
{ }


int32_t Song::Reference() const
{
   return reference_;
}

void Song::IncrementReference()
{
   ++reference_;
}

void Song::DecrementReference()
{
   --reference_;
}


void Song::SetArtist(const char * artist)
{
   if (artist != NULL)
   {
      artist_ = artist;
   }
   else
   {
      artist_ = "Unknown";
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
      album_ = "Unknown";
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
      title_ = "Unknown";
   }
}

std::string const & Song::Title() const
{
   return title_;
}

void Song::SetURI(const char * uri)
{
   if (uri != NULL)
   {
      uri_ = uri;
   }
   else
   {
      uri_ = "Unknown";
   }
}

std::string const & Song::URI() const
{
   return uri_;
}

void Song::SetDuration(int32_t duration)
{
   duration_ = duration;
}

int32_t Song::Duration() const
{
   return duration_;
}

std::string Song::DurationString() const
{
   char duration[32];

   uint32_t const minutes = static_cast<uint32_t>(duration_ / 60);
   uint32_t const seconds = (duration_ - (minutes * 60));

   snprintf(duration, 32, "%2d:%.2d", minutes, seconds);

   return (std::string(duration));
}


std::string Song::PlaylistDescription() const
{
   std::string fullDescription(artist_ + " - " + title_ + " " + DurationString());
   return fullDescription;
}

std::string Song::FullDescription() const
{
   std::string fullDescription(artist_ + " - " + title_ + " " + album_ + " " + DurationString());
   return fullDescription;
}
