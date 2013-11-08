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
#include "compiler.hpp"
#include "settings.hpp"
#include "song.hpp"

#include "buffer/library.hpp"
#include "buffer/list.hpp"

#include <vector>

namespace Ui   { class DirectoryWindow; }

namespace Mpc
{
   class  Client;
   class  ClientState;
   class  Directory;
   class  DirectoryEntry;

   typedef std::vector<DirectoryEntry *> DirectoryEntryVector;

   class DirectoryEntry
   {
   public:
      DirectoryEntry(EntryType Type, std::string Name, std::string Path, Mpc::Song * Song = NULL) :
         type_    (Type),
         name_    (Name),
         path_    (Path),
         song_    (Song)
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

   class DirectoryComparator
   {
      public:
      bool operator() (DirectoryEntry * i, DirectoryEntry * j)
      {
         if (i->name_ == "..")
         {
            return true;
         }
         else if (j->name_ == "..")
         {
            return false;
         }
         else if (i->type_ == j->type_)
         {
            return (i->name_ < j->name_);
         }
         else if (i->type_ == PathType)
         {
            return true;
         }
         else if (j->type_ == PathType)
         {
            return false;
         }

         return (i->name_ < j->name_);
      };
   };


   // Directory class
   class Directory : public Main::Buffer<DirectoryEntry *>
   {
      friend class Mpc::Song;

   public:
      using Main::Buffer<DirectoryEntry *>::Add;
      using Main::Buffer<DirectoryEntry *>::Sort;

      Directory();
      ~Directory();

   public:
      static std::string FileFromURI(std::string const & URI);
      static std::string DirectoryFromURI(std::string const & URI);
      static bool IsChildPath(std::string const & Parent, std::string const & Child);
      static std::string ParentPath(std::string const & Path);


   public:
      std::string CurrentDirectory();
      void ChangeDirectory(std::string New);
      void ChangeDirectory(DirectoryEntry & New);

      void Clear(bool fullClear = false);
      void Add(std::string directory);
      void AddChild(std::string directory);
      void Add(Mpc::Song * song);
      void AddPlaylist(Mpc::List playlist);
      void AddToPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, Mpc::ClientState & clientState, uint32_t position);
      void RemoveFromPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, Mpc::ClientState & clientState, uint32_t position);

      uint32_t TotalReferences(std::string const & Path) const
      {
         uint32_t Result = References(Path);
         std::vector<std::string> const CPaths = ChildPaths(Path);

         FOREACH(auto CPath, CPaths)
         {
            Result += References(CPath);
         }
         return Result;
      }

      std::vector<Mpc::Song *> AllChildSongs(std::string const & Path) const
      {
         std::vector<Mpc::Song *> Result;
         std::vector<std::string> const CPaths = ChildPaths(Path);
         std::vector<Mpc::Song *> const OwnSongs = Songs(Path);

         FOREACH(auto CPath, CPaths)
         {
            std::vector<Mpc::Song *> Children = Songs(CPath);
            Result.insert(Result.end(), Children.begin(), Children.end());
         }

         Result.insert(Result.end(), OwnSongs.begin(), OwnSongs.end());
         return Result;
      }

      void Sort()
      {
         DirectoryComparator sorter;
         Main::Buffer<DirectoryEntry *>::Sort(sorter);
      }

      std::vector<std::string> const & Paths() { return paths_; }

   private:
      void AddEntry(std::string fullPath);
      void AddToPlaylist(Mpc::Client & client, Mpc::ClientState & clientState, Mpc::DirectoryEntry const * const entry, int32_t position = -1);
      void RemoveFromPlaylist(Mpc::Client & client, Mpc::ClientState & clientState, Mpc::DirectoryEntry const * const entry);
      void DeleteEntry(DirectoryEntry * const entry);

      void AddedToPlaylist(std::string URI)
      {
         ++references_[DirectoryFromURI(URI)];
      }
      void RemovedFromPlaylist(std::string URI)
      {
         --references_[DirectoryFromURI(URI)];
      }
      uint32_t References(std::string const & Path) const
      {
         std::map<std::string, int>::const_iterator it = references_.find(Path);
         return (it != references_.end()) ? it->second : 0;
      }
      std::vector<Mpc::Song *> Songs(std::string const & Path) const
      {
         static std::vector<Mpc::Song *> emptyvector;
         auto const it = songs_.find(Path);
         return (it != songs_.end()) ? it->second : emptyvector;
      }
      std::vector<std::string> ChildPaths(std::string const & Path) const
      {
         static std::vector<std::string> emptyvector;
         auto const it = children_.find(Path);
         return (it != children_.end()) ? it->second : emptyvector;
      }

   private:
      std::vector<std::string>                         paths_;
      std::map<std::string, int >                      references_;
      std::map<std::string, std::vector<Mpc::Song *> > songs_;
      std::map<std::string, std::vector<std::string> > playlists_;
      std::map<std::string, std::vector<std::string> > children_;
      std::string directory_;
   };
}

#endif
/* vim: set sw=3 ts=3: */
