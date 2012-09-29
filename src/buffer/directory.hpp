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

   directory.hpp - handling of the mpd music directory
   */

#ifndef __UI__DIRECTORY
#define __UI__DIRECTORY

#include "algorithm.hpp"
#include "buffer.hpp"
#include "settings.hpp"
#include "song.hpp"

#include "buffer/library.hpp"
#include "buffer/list.hpp"

#include <vector>

namespace Ui   { class DirectoryWindow; }

namespace Mpc
{
   class  Client;
   class  Directory;
   class  DirectoryEntry;

   typedef std::vector<DirectoryEntry *> DirectoryEntryVector;

   class DirectoryEntry
   {
   public:
      friend class Directory;

      DirectoryEntry() :
         type_    (PathType),
         name_    (""),
         path_    (""),
         song_    (NULL)
      { }

   private:
      DirectoryEntry(DirectoryEntry & entry);
      DirectoryEntry & operator=(DirectoryEntry & entry);

   public:
      EntryType          type_;
      std::string        name_;
      std::string        path_;
      Mpc::Song *        song_;
   };


   // Directory class
   class Directory : public Main::Buffer<DirectoryEntry *>
   {
   public:
      using Main::Buffer<DirectoryEntry *>::Add;

      Directory();
      ~Directory();

   public:
      std::string CurrentDirectory();
      void ChangeDirectory(std::string New);
      void ChangeDirectory(DirectoryEntry & New);

      void Clear(bool fullClear = false);
      void Add(std::string directory);
      void Add(Mpc::Song * song);
      void AddPlaylist(Mpc::List playlist);
      void AddToPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, uint32_t position);
      void RemoveFromPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, uint32_t position);

   private:
      void AddEntry(std::string fullPath);
      void AddToPlaylist(Mpc::Client & client, Mpc::DirectoryEntry const * const entry, int32_t position = -1);
      void RemoveFromPlaylist(Mpc::Client & client, Mpc::DirectoryEntry const * const entry);
      void DeleteEntry(DirectoryEntry * const entry);

      typedef Main::CallbackObject<Mpc::Directory, Directory::BufferType> CallbackObject;
      typedef Main::CallbackFunction<Directory::BufferType> CallbackFunction;

   private:
      std::vector<std::string> paths_;
      std::map<std::string, std::vector<Mpc::Song *> > songs_;
      std::map<std::string, std::vector<std::string> > playlists_;
      std::string directory_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
