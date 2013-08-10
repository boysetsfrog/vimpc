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

#ifdef HAVE_TAGLIB_H
#include <taglib/taglib.h>
#include <taglib/fileref.h>
#endif

namespace Mpc
{
   namespace Tag
   {
#ifdef TAG_SUPPORT
      void SetArtist(std::string const & filePath, const char * artist)
      {
         TagLib::FileRef file(filePath.c_str());

         if ((file.isNull() == false) && (file.tag() != NULL))
         {
            file.tag()->setArtist(artist);
            file.save();
         }
      }

      void SetAlbum(std::string const & filePath, const char * album)
      {
         TagLib::FileRef file(filePath.c_str());

         if ((file.isNull() == false) && (file.tag() != NULL))
         {
            file.tag()->setAlbum(album);
            file.save();
         }
      }

      void SetTitle(std::string const & filePath, const char * title)
      {
         TagLib::FileRef file(filePath.c_str());

         if ((file.isNull() == false) && (file.tag() != NULL))
         {
            file.tag()->setTitle(title);
            file.save();
         }
      }

      void SetTrack(std::string const & filePath, const char * track)
      {
         TagLib::FileRef file(filePath.c_str());

         if ((file.isNull() == false) && (file.tag() != NULL))
         {
            //file.tag()->setArtist(artist);
            file.save();
         }
      }

      std::string Artist(std::string const & filePath);
      std::string Album(std::string const & filePath);
      std::string Title(std::string const & filePath);
      std::string Track(std::string const & filePath);

#else
      void SetArtist(std::string const & filePath, const char * artist) {}
      void SetTitle(std::string const & filePath, const char * title)   {}
      void SetTrack(std::string const & filePath, const char * track)   {}
      void SetAlbum(std::string const & filePath, const char * album)   {}

      std::string Artist(std::string const & filePath) { return ""; }
      std::string Album(std::string const & filePath)  { return ""; }
      std::string Title(std::string const & filePath)  { return ""; }
      std::string Track(std::string const & filePath)  { return ""; }
#endif
   };
}

#endif
/* vim: set sw=3 ts=3: */
