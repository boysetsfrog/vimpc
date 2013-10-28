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

   tag.hpp - tag information and editting
   */

#ifndef __MPC_TAG
#define __MPC_TAG

#include "config.h"
#include "window/debug.hpp"
#include "window/error.hpp"

#ifdef HAVE_TAGLIB_H
#include <taglib/taglib.h>
#include <taglib/fileref.h>
#endif

namespace Mpc
{
   namespace Tag
   {
      void SetArtist(Mpc::Song * song, std::string const & filePath, const char * artist);
      void SetTitle(Mpc::Song * song, std::string const & filePath, const char * title);
      void SetTrack(Mpc::Song * song, std::string const & filePath, const char * track);
      void SetAlbum(Mpc::Song * song, std::string const & filePath, const char * album);

#ifdef TAG_SUPPORT
      void SetArtist(Mpc::Song * song, std::string const & filePath, const char * artist)
      {
         TagLib::FileRef file(filePath.c_str());

         if ((file.isNull() == false) && (file.tag() != NULL))
         {
            file.tag()->setArtist(artist);

            if (file.save() == true)
            {
               Debug("Edit artist tag of %s to %s", filePath.c_str(), artist);
               song->SetArtist(artist);
            }
         }
         else
         {
            ErrorString(ErrorNumber::FileNotFound);
         }
      }

      void SetAlbum(Mpc::Song * song, std::string const & filePath, const char * album)
      {
         TagLib::FileRef file(filePath.c_str());

         if ((file.isNull() == false) && (file.tag() != NULL))
         {
            file.tag()->setAlbum(album);

            if (file.save() == true)
            {
               Debug("Edit album tag of %s to %s", filePath.c_str(), album);
               song->SetAlbum(album);
            }
         }
         else
         {
            ErrorString(ErrorNumber::FileNotFound);
         }
      }

      void SetTitle(Mpc::Song * song, std::string const & filePath, const char * title)
      {
         TagLib::FileRef file(filePath.c_str());

         if ((file.isNull() == false) && (file.tag() != NULL))
         {
            file.tag()->setTitle(title);

            if (file.save() == true)
            {
               Debug("Edit title tag of %s to %s", filePath.c_str(), title);
               song->SetTitle(title);
            }
         }
         else
         {
            ErrorString(ErrorNumber::FileNotFound);
         }
      }

      void SetTrack(Mpc::Song * song, std::string const & filePath, const char * track)
      {
         TagLib::FileRef file(filePath.c_str());

         if ((file.isNull() == false) && (file.tag() != NULL))
         {
            file.tag()->setTrack(atoi(track));

            if (file.save() == true)
            {
               Debug("Edit track tag of %s to %s", filePath.c_str(), track);
               song->SetTrack(track);
            }
         }
         else
         {
            ErrorString(ErrorNumber::FileNotFound);
         }
      }

      std::string Artist(std::string const & filePath);
      std::string Album(std::string const & filePath);
      std::string Title(std::string const & filePath);
      std::string Track(std::string const & filePath);

#else
      void SetArtist(Mpc::Song * song, std::string const & filePath, const char * artist) {}
      void SetTitle(Mpc::Song * song, std::string const & filePath, const char * title)   {}
      void SetTrack(Mpc::Song * song, std::string const & filePath, const char * track)   {}
      void SetAlbum(Mpc::Song * song, std::string const & filePath, const char * album)   {}

      std::string Artist(std::string const & filePath) { return ""; }
      std::string Album(std::string const & filePath)  { return ""; }
      std::string Title(std::string const & filePath)  { return ""; }
      std::string Track(std::string const & filePath)  { return ""; }
#endif
   };
}

#endif
/* vim: set sw=3 ts=3: */
