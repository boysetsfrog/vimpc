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

   directory.cpp - handling of the mpd music directory
   */

#include "directory.hpp"

#include "algorithm.hpp"
#include "clientstate.hpp"
#include "mpdclient.hpp"
#include "playlist.hpp"

#include "window/debug.hpp"

#include <algorithm>

using namespace Mpc;

Directory::Directory() :
   directory_("")
{
}

Directory::~Directory()
{
   Clear();
}

std::string Directory::CurrentDirectory()
{
   return directory_;
}

void Directory::ChangeDirectory(std::string New)
{
   Clear();

   bool found = false;

   for (auto path : paths_)
   {
      if (path == New)
      {
         found = true;
      }
   }

   if (found == false)
   {
      New = "";
   }

   if (New != "")
   {
      if (New.find("/") != std::string::npos)
      {
         directory_ = New.substr(0, New.find_last_of("/"));
      }
      else
      {
         directory_ = "";
      }

      AddEntry(directory_);
   }

   directory_ = New;

   for (auto path : paths_)
   {
      if (path != New)
      {
         AddEntry(path);
      }
   }

   std::vector<Mpc::Song *> const & songs = songs_[directory_];

   for (auto song : songs)
   {
      Mpc::DirectoryEntry * const entry =
         new Mpc::DirectoryEntry(Mpc::SongType, FileFromURI(song->URI()), directory_, song);
      Add(entry);
   }

   if ((Main::Settings::Instance().Get(Setting::ShowLists) == true))
   {
      std::vector<std::string> const & playlists = playlists_[directory_];

      for (auto playlist : playlists)
      {
         Mpc::DirectoryEntry * const entry =
            new Mpc::DirectoryEntry(Mpc::PlaylistType, FileFromURI(playlist), directory_);
         Add(entry);
      }
   }

   Sort();
}

void Directory::ChangeDirectory(DirectoryEntry & New)
{
   ChangeDirectory(New.path_);
}

void Directory::Clear(bool fullClear)
{
   if (fullClear == true)
   {
      paths_.clear();
      songs_.clear();
      children_.clear();
      playlists_.clear();
   }

   while (Size() > 0)
   {
      DirectoryEntry * entry = Get(0);
      Remove(0, 1);
      delete entry;
   }
}

void Directory::Add(std::string directory)
{
   AddEntry(directory);
   paths_.push_back(directory);
   AddChild(directory);
}

void Directory::AddChild(std::string directory)
{
   std::string Parent = "";
   std::string Path = directory;

   size_t i = 0;

   while (i != std::string::npos)
   {
      i = Path.find("/", i);

      if (i != std::string::npos)
      {
         Parent = Path.substr(0, i);
         children_[Parent].push_back(directory);
         ++i;
      }
   }
}

void Directory::Add(Mpc::Song * song)
{
   std::string Path = song->URI();

   if (Path.find("/") != std::string::npos)
   {
      Path = Path.substr(0, Path.find_last_of("/"));
   }
   else
   {
      Path = "";
   }

   songs_[Path].push_back(song);
}

void Directory::AddPlaylist(Mpc::List playlist)
{
   std::string Path = playlist.path_;

   if (Path.find("/") != std::string::npos)
   {
      Path = Path.substr(0, Path.find_last_of("/"));
   }
   else
   {
      Path = "";
   }

   playlists_[Path].push_back(playlist.path_);
}


void Directory::AddEntry(std::string fullPath)
{
   std::string directory = fullPath;

   if (fullPath == directory_)
   {
      directory = "..";
   }
   else if (fullPath.find("/") != std::string::npos)
   {
      directory = fullPath.substr(fullPath.find_last_of("/") + 1);
   }

   if (((directory.size() >= 1) && (directory[0] != '.')) ||
         ((directory.size() >= 2) && directory[1] == '.'))
   {
      if (((directory_ != "") &&
               (fullPath.find(directory_ + "/") == 0) &&
               (fullPath.find(directory_ + "/") + directory_.size() == fullPath.find_last_of("/"))) ||
            ((directory_ == "") && (fullPath.find('/') == std::string::npos)) ||
            (directory == ".."))
      {
         Mpc::DirectoryEntry * entry = new Mpc::DirectoryEntry(Mpc::PathType, directory, fullPath);
         Add(entry);
      }
   }
}


void Directory::AddToPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, Mpc::ClientState & clientState, uint32_t position)
{
   if (position < Size())
   {
      if (Collection == Mpc::Song::Single)
      {
         AddToPlaylist(client, clientState, Get(position));
      }
      else
      {
         Mpc::CommandList list(client);

         for (uint32_t i = 0; i < Size(); ++i)
         {
            AddToPlaylist(client, clientState, Get(i));
         }
      }
   }
}

void Directory::RemoveFromPlaylist(Mpc::Song::SongCollection Collection, Mpc::Client & client, Mpc::ClientState & clientState, uint32_t position)
{
   if (position < Size())
   {
      if (Collection == Mpc::Song::Single)
      {
         RemoveFromPlaylist(client, clientState, Get(position));
      }
      else
      {
         Mpc::CommandList list(client);

         for (uint32_t i = 0; i < Size(); ++i)
         {
            RemoveFromPlaylist(client, clientState, Get(i));
         }
      }
   }
}

void Directory::AddToPlaylist(Mpc::Client & client, Mpc::ClientState & clientState, Mpc::DirectoryEntry const * const entry, int32_t position)
{
   if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL))
   {
      if (position != -1)
      {
         client.Add(*(entry->song_), position);
      }
      else if ((Main::Settings::Instance().Get(Setting::AddPosition) == Setting::AddEnd) ||
               (clientState.GetCurrentSongPos() == -1))
      {
         client.Add(*(entry->song_));
      }
      else
      {
         client.Add(*(entry->song_), clientState.GetCurrentSongPos() + 1);
      }
   }
   else if (entry->type_ == Mpc::PlaylistType)
   {
      std::string const path((entry->path_ == "") ? "" : entry->path_ + "/");
      client.AppendPlaylist(path + entry->name_);
   }
   else if (entry->type_ == Mpc::PathType)
   {
      client.Add(entry->path_);
   }
}

void Directory::RemoveFromPlaylist(Mpc::Client & client, Mpc::ClientState & clientState, Mpc::DirectoryEntry const * const entry)
{
   if ((entry->type_ == Mpc::SongType) && (entry->song_ != NULL))
   {
      int32_t PlaylistIndex = Main::Playlist().Index(entry->song_);

      if (PlaylistIndex >= 0)
      {
         client.Delete(PlaylistIndex);
      }
   }
   else if (entry->type_ == Mpc::PlaylistType)
   {
      std::string const path((entry->path_ == "") ? "" : entry->path_ + "/");
      client.PlaylistContentsForRemove(path + entry->name_);
   }
   else if (entry->type_ == Mpc::PathType)
   {
      std::vector<Mpc::Song *> const ChildSongs = AllChildSongs(entry->path_);

      if (ChildSongs.size() > 0)
      {
         Mpc::CommandList list(client, (ChildSongs.size() > 1));

         for (uint32_t i = 0; i < ChildSongs.size(); ++i)
         {
            int32_t PlaylistIndex = Main::Playlist().Index(ChildSongs[i]);

            if (PlaylistIndex >= 0)
            {
               client.Delete(PlaylistIndex);
            }
         }
      }
   }
}

/* static */ bool Directory::IsChildPath(std::string const & Parent, std::string const & Child)
{
   return ((Parent == "") || (Child.find(Parent + "/") != std::string::npos));
}

/* static */ std::string Directory::ParentPath(std::string const & Path)
{
   std::string Parent = "";

   if (Path.find("/") != std::string::npos)
   {
      Parent = Path.substr(0, Path.find_last_of("/"));
   }

   return Parent;
}

/* static */ std::string Directory::FileFromURI(std::string const & URI)
{
   std::string File = URI;

   if (File.find("/") != std::string::npos)
   {
      File = File.substr(File.find_last_of("/") + 1);
   }

   return File;
}

/* static */ std::string Directory::DirectoryFromURI(std::string const & URI)
{
   std::string Directory = "";

   if (URI.find("/") != std::string::npos)
   {
      Directory = URI.substr(0, URI.find_last_of("/"));
   }

   return Directory;
}

/* vim: set sw=3 ts=3: */
