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

#include "buffers.hpp"
#include "buffer/directory.hpp"
#include "buffer/library.hpp"

const std::string UnknownArtist = "Unknown Artist";
const std::string UnknownAlbum  = "Unknown Album";
const std::string UnknownTitle  = "Unknown";
const std::string UnknownURI    = "Unknown";
const std::string UnknownGenre  = "Unknown";
const std::string UnknownTrack  = "";
const std::string UnknownDate   = "Unknown";
const std::string UnknownDisc   = "";

std::vector<std::string> Mpc::Song::Artists;
std::map<std::string, uint32_t> Mpc::Song::ArtistMap;

std::vector<std::string> Mpc::Song::Albums;
std::map<std::string, uint32_t> Mpc::Song::AlbumMap;

std::vector<std::string> Mpc::Song::Tracks;
std::map<std::string, uint32_t> Mpc::Song::TrackMap;

std::vector<std::string> Mpc::Song::Genres;
std::map<std::string, uint32_t> Mpc::Song::GenreMap;

std::vector<std::string> Mpc::Song::Dates;
std::map<std::string, uint32_t> Mpc::Song::DateMap;

std::vector<std::string> Mpc::Song::Discs;
std::map<std::string, uint32_t> Mpc::Song::DiscMap;

std::map<char, Mpc::Song::SongFunction> Mpc::Song::SongInfo;

using namespace Mpc;

Song::Song() :
   reference_   (0),
   artist_      (-1),
   albumArtist_ (-1),
   album_       (-1),
   track_       (-1),
   genre_       (-1),
   date_        (-1),
   disc_        (-1),
   duration_    (0),
   virtualEnd_  (0),
   uri_         (""),
   title_       (""),
   lastFormat_  (""),
   formatted_   (""),
   entry_       (NULL)
{ }

Song::Song(Song const & song) :
   reference_   (0),
   artist_      (song.artist_),
   albumArtist_ (song.albumArtist_),
   album_       (song.album_),
   track_       (song.track_),
   genre_       (song.genre_),
   date_        (song.date_),
   disc_        (song.disc_),
   duration_    (song.duration_),
   uri_         (song.URI()),
   title_       (song.Title()),
   lastFormat_  (song.lastFormat_),
   formatted_   (song.formatted_)
{
   SetDuration(duration_);
}

Song::~Song()
{
   reference_ = 0;

   if (entry_ != NULL)
   {
      entry_->song_ = NULL;
   }
}


int32_t Song::Reference() const
{
   return reference_;
}

/* static */ void Song::IncrementReference(Song * song)
{
   if (song) {
       song->reference_ += 1;

       if ((song->entry_ != NULL) && (song->reference_ == 1))
       {
          song->entry_->AddedToPlaylist();
          Main::Directory().AddedToPlaylist(song->URI());
       }
   }
}

/* static */ void Song::DecrementReference(Song * song)
{
   if (song) {
      song->reference_ -= 1;

      if ((song->entry_ != NULL) && (song->reference_ == 0))
      {
          song->entry_->RemovedFromPlaylist();
          Main::Directory().RemovedFromPlaylist(song->URI());
      }
   }
}

/* static */ void Song::SwapThe(std::string & String)
{
   static const Regex::RE exp = Regex::RE("^\\s*[tT][hH][eE]\\s+");

   if (exp.Replace("", String))
   {
      String += ", The";
   }
}



void Song::Set(const char * newVal, int32_t & oldVal, std::vector<std::string> & Values, std::map<std::string, uint32_t> & Indexes)
{
   lastFormat_ = "";

   if (newVal == NULL)
   {
      oldVal = -1;
   }
   else
   {
      auto it = Indexes.find(newVal);

      if (it == Indexes.end())
      {
         oldVal = Values.size();
         Indexes[std::string(newVal)] = oldVal;
         Values.push_back(newVal);
      }
      else
      {
         oldVal = it->second;
      }
   }
}

void Song::SetArtist(const char * artist)
{
   Set(artist, artist_, Artists, ArtistMap);
}

std::string const & Song::Artist() const
{
   if ((artist_ >= 0) && (artist_ < static_cast<int32_t>(Artists.size())))
   {
      return Artists.at(artist_);
   }

   return UnknownArtist;
}

void Song::SetAlbumArtist(const char * albumArtist)
{
   Set(albumArtist, albumArtist_, Artists, ArtistMap);
}

std::string const & Song::AlbumArtist() const
{
   if ((albumArtist_ >= 0) && (albumArtist_ < static_cast<int32_t>(Artists.size())))
   {
      return Artists.at(albumArtist_);
   }

   if ((artist_ >= 0) && (artist_ < static_cast<int32_t>(Artists.size())))
   {
      return Artists.at(artist_);
   }

   return UnknownArtist;
}

void Song::SetAlbum(const char * album)
{
   Set(album, album_, Albums, AlbumMap);
}

std::string const & Song::Album() const
{
   if ((album_ >= 0) && (album_ < static_cast<int32_t>(Albums.size())))
   {
      return Albums.at(album_);
   }

   return UnknownAlbum;
}

void Song::SetTitle(const char * title)
{
   lastFormat_ = "";

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
   Set(track, track_, Tracks, TrackMap);
}

std::string const & Song::Track() const
{
   if ((track_ >= 0) && (track_ < static_cast<int32_t>(Tracks.size())))
   {
      return Tracks.at(track_);
   }

   return UnknownTrack;
}

void Song::SetURI(const char * uri)
{
   lastFormat_ = "";

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

void Song::SetGenre(const char * genre)
{
   Set(genre, genre_, Genres, GenreMap);
}

std::string const & Song::Genre() const
{
   if ((genre_ >= 0) && (genre_ < static_cast<int32_t>(Genres.size())))
   {
      return Genres.at(genre_);
   }

   return UnknownGenre;
}

void Song::SetDate(const char * date)
{
   Set(date, date_, Dates, DateMap);
}

std::string const & Song::Date() const
{
   if ((date_ >= 0) && (date_ < static_cast<int32_t>(Dates.size())))
   {
      return Dates.at(date_);
   }

   return UnknownDate;
}

void Song::SetDisc(const char * disc)
{
   Set(disc, disc_, Discs, DiscMap);
}

std::string const & Song::Disc() const
{
   if ((disc_ >= 0) && (disc_ < static_cast<int32_t>(Discs.size())))
   {
      return Discs.at(disc_);
   }

   return UnknownDisc;
}

void Song::SetDuration(int32_t duration)
{
   lastFormat_ = "";
   duration_ = duration;
}

int32_t Song::Duration() const
{
   return duration_;
}

void Song::SetVirtualEnd(int32_t end)
{
   virtualEnd_ = end;
}

int32_t Song::VirtualEnd() const
{
   return virtualEnd_;
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
   static std::string Result;
   static char cduration[32];

   uint32_t const minutes = static_cast<uint32_t>(duration_ / 60);
   uint32_t const seconds = (duration_ - (minutes * 60));

   snprintf(cduration, 32, "%2d:%.2d", minutes, seconds);
   Result = std::string(cduration);
   return Result;
}

std::string Song::FormatString(std::string fmt) const
{
   if (lastFormat_ == fmt)
   {
      return formatted_;
   }

   lastFormat_ = fmt;
   std::string::const_iterator it = fmt.begin();
   formatted_ = ParseString(it, true);

   return formatted_;
}

/* static */ void Song::RepopulateSongFunctions()
{
   SongInfo['b'] = &Mpc::Song::Album;
   SongInfo['B'] = &Mpc::Song::Album;
   SongInfo['l'] = &Mpc::Song::DurationString;
   SongInfo['t'] = &Mpc::Song::Title;
   SongInfo['n'] = &Mpc::Song::Track;
   SongInfo['f'] = &Mpc::Song::URI;
   SongInfo['d'] = &Mpc::Song::Date;
   SongInfo['c'] = &Mpc::Song::Disc;

   SongInfo['r'] = &Mpc::Song::Artist;
   SongInfo['R'] = &Mpc::Song::Artist;
   SongInfo['m'] = &Mpc::Song::AlbumArtist;
   SongInfo['M'] = &Mpc::Song::AlbumArtist;

   if (Main::Settings::Instance().Get(Setting::AlbumArtist) == true)
   {
      SongInfo['a'] = &Mpc::Song::AlbumArtist;
      SongInfo['A'] = &Mpc::Song::AlbumArtist;
   }
   else
   {
      SongInfo['a'] = &Mpc::Song::Artist;
      SongInfo['A'] = &Mpc::Song::Artist;
   }
}

std::string Song::ParseString(std::string::const_iterator & it, bool valid) const
{
   std::string result;
   bool tmp_valid = false;

   if (SongInfo.empty() == true)
   {
      Mpc::Song::RepopulateSongFunctions();
   }

   do
   {
      if (*it == '\\')
      {
         *++it;
         result += *it;
         continue;
      }
      else if (*it == '{')
      {
         *++it;
         uint32_t len = result.length();
         result += ParseString(it, valid);

         if (result.length() != len) 
         {
             tmp_valid = true;
         }
         else 
         {
             tmp_valid = false;
         }
      }
      else if (*it == '}')
      {
         valid = (valid) ? tmp_valid : valid;
         if (valid) break;
         else return "";
      }
      else if (*it == '|')
      {
         valid = (valid) ? tmp_valid : valid;
         valid = !valid;
      }
      else if (*it == '%')
      {
         *++it;
         if (*it == '%')
         {
            result += *it;
         }
         else if ((*it == 'a') || (*it == 'A') ||
                  (*it == 'b') || (*it == 'B') ||
                  (*it == 'r') || (*it == 'R') ||
                  (*it == 'm') || (*it == 'M') ||
                  (*it == 'l') || (*it == 't') ||
                  (*it == 'n') || (*it == 'f') ||
                  (*it == 'd') || (*it == 'c'))
         {
            SongFunction Function = SongInfo[*it];
            std::string val = (*this.*Function)();

            if ((*it == 'B') || (*it == 'A') ||
                (*it == 'R') || (*it == 'M'))
            {
               SwapThe(val);
            }

            if ((val == "") || (val.substr(0, strlen("Unknown")) == "Unknown"))
            {
               result += val;
            }
            else
            {
               tmp_valid = true;
               result += val;
            }
         }
      }
      else
      {
         result += *it;
      }
   } while (*++it);

   return result;
}

/* vim: set sw=3 ts=3: */
