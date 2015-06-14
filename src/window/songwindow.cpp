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

   songwindow.hpp - handling of the mpd library but with a playlist interface
   */

#include "songwindow.hpp"

#include "buffers.hpp"
#include "clientstate.hpp"
#include "errorcodes.hpp"
#include "mpdclient.hpp"
#include "regex.hpp"
#include "settings.hpp"
#include "screen.hpp"

#include "buffer/library.hpp"
#include "buffer/list.hpp"
#include "buffer/playlist.hpp"
#include "mode/search.hpp"
#include "window/error.hpp"
#include "window/infowindow.hpp"

using namespace Ui;

SongWindow::SongWindow(Main::Settings const & settings, Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Ui::Search const & search, std::string name) :
   SelectWindow     (settings, screen, name),
   settings_        (settings),
   client_          (client),
   clientState_     (clientState),
   search_          (search),
   browse_          ()
{
}

SongWindow::~SongWindow()
{
}

void SongWindow::Add(Mpc::Song * song)
{
   if (song != NULL)
   {
      Buffer().Add(song);
   }
}

void SongWindow::AddToPlaylist(uint32_t position)
{
   if ((position < BufferSize()) && (Buffer().Get(position) != NULL))
   {
      if ((settings_.Get(Setting::AddPosition) == Setting::AddEnd) ||
          (clientState_.GetCurrentSongPos() == -1))
      {
         client_.Add(*(Buffer().Get(position)));
      }
      else
      {
         client_.Add(*(Buffer().Get(position)), clientState_.GetCurrentSongPos() + 1);
      }
   }
}

std::string SongWindow::SearchPattern(uint32_t id) const
{
   if (id < Buffer().Size())
   {
      return Buffer().Get(id)->FormatString(settings_.Get(Setting::SongFormat));
   }
   return "";
}


void SongWindow::Print(uint32_t line) const
{
#if 0
   uint32_t printLine = line + FirstLine();
   WINDOW * window    = N_WINDOW();
   Mpc::Song * song   = (printLine < BufferSize()) ? Buffer().Get(printLine) : NULL;
   int32_t  colour    = DetermineColour(printLine, song);

   // Reverse the colours to indicate the selected song
   if ((IsSelected(printLine) == true) && (song != NULL))
   {
      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattron(window, COLOR_PAIR(colour));
      }

      wattron(window, A_REVERSE);
   }

   // Blank the current line, this ensures that selected line is correct colour
   mvwhline(window, line, 0, ' ', screen_.MaxColumns());

   if (song != NULL)
   {
      wmove(window, line, 0);

      if (settings_.Get(Setting::SongNumbers) == true)
      {
         PrintId(printLine);
      }
      else
      {
         PrintBlankId();
      }

      PrintSong(line, printLine, colour, settings_.Get(Setting::SongFormat), song);
   }

   if ((IsSelected(printLine) == true) && (song != NULL))
   {
      if (settings_.Get(Setting::ColourEnabled) == true)
      {
         wattroff(window, COLOR_PAIR(colour));
      }

      wattroff(window, A_REVERSE);
   }
#else
   SelectWindow::Print(line);
#endif
}


void SongWindow::Left(Ui::Player & player, uint32_t count)
{
   //player.SkipSong(Ui::Player::Previous, count);
}

void SongWindow::Right(Ui::Player & player, uint32_t count)
{
   //player.SkipSong(Ui::Player::Next, count);
}

void SongWindow::Confirm()
{
   if (Buffer().Size() > CurrentLine())
   {
      int64_t pos1, pos2;
      uint32_t count = GetPositions(pos1, pos2);

      if (count == 1)
      {
         AddToPlaylist(CurrentLine());
      }
      else
      {
         AddLine(pos1, count, false);
      }


      if (settings_.Get(Setting::AddPosition) == Setting::AddEnd)
      {
         client_.Play(static_cast<uint32_t>(Main::Playlist().Size()));
      }
      else
      {
         client_.Play(static_cast<uint32_t>(clientState_.GetCurrentSongPos() + 1));
      }
   }

   SelectWindow::Confirm();
}

uint32_t SongWindow::Current() const
{
   int32_t current       = CurrentLine();
   int32_t currentSongId = clientState_.GetCurrentSongPos();

   if ((currentSongId >= 0) && (currentSongId < static_cast<int32_t>(Main::Playlist().Size())))
   {
      current = Buffer().Index(Main::Playlist().Get(currentSongId));
   }

   if (current == -1)
   {
      current = CurrentLine();
   }

   return current;
}

uint32_t SongWindow::Playlist(int count) const
{
   //! \todo could use some tidying up
   int32_t line = CurrentLine();

   if ((line >= 0) && (Buffer().Size() > 0))
   {
      if ((count > 0) && (line >= 0) && (Buffer().Size() > 0))
      {
         for (uint32_t i = CurrentLine() + 1; i < BufferSize(); ++i)
         {
            if (Buffer().Get(i)->Reference() > 0)
            {
               --count;
               line = i;
            }

            if (count == 0)
            {
               break;
            }
         }
      }
      else
      {
         for (int32_t i = CurrentLine() - 1; i >= 0; --i)
         {
            if (Buffer().Get(i)->Reference() > 0)
            {
               ++count;
               line = i;
            }

            if (count == 0)
            {
               break;
            }
         }
      }
   }

   return line;
}

void SongWindow::AddLine(uint32_t line, uint32_t count, bool scroll)
{
   if (clientState_.Connected() == true)
   {
      int64_t pos1, pos2;
      uint32_t const posCount = GetPositions(pos1, pos2);

      if (posCount > 1)
      {
         count = posCount;
         line  = pos1;
      }

      {

         if (settings_.Get(Setting::AddPosition) == Setting::AddEnd)
         {
            std::vector<Mpc::Song *> songs;

            for (uint32_t i = 0; i < count; ++i)
            {
               //AddToPlaylist(line + i);
               uint32_t const position = line + i;

               if ((position < BufferSize()) && (Buffer().Get(position) != NULL))
               {
                  songs.push_back(Buffer().Get(position));
               }
            }

            Debug("Queueing up a client add of %d songs", songs.size());
            client_.Add(songs);
         }
         else
         {
            Mpc::CommandList list(client_, (count > 1));

            for (int32_t i = count - 1; i >= 0; --i)
            {
               AddToPlaylist(line + i);
            }
         }
      }

      if ((scroll == true) && (posCount == 1))
      {
         Scroll(count);
      }
   }

   SelectWindow::AddLine(line, count, scroll);
}

void SongWindow::AddAllLines()
{
   AddLine(0, BufferSize(), false);
}

void SongWindow::CropLine(uint32_t line, uint32_t count, bool scroll)
{
   DeleteLine(line, count, scroll);
}

void SongWindow::CropAllLines()
{
   if (BufferSize() > 0)
   {
      DeleteLine(CurrentLine(), BufferSize() - CurrentLine(), false);
   }
}

void SongWindow::DeleteLine(uint32_t line, uint32_t count, bool scroll)
{
   if (Main::Playlist().Size() > 0)
   {
      Main::PlaylistPasteBuffer().Clear();
   }

   if (clientState_.Connected() == true)
   {
      int64_t pos1, pos2;
      uint32_t const posCount = GetPositions(pos1, pos2);

      if (posCount > 1)
      {
         count  = posCount;
         line   = pos1;
      }

      {
         Mpc::CommandList list(client_, (count > 1));

         for (uint32_t i = 0; i < count; ++i)
         {
            int32_t index = line;

            if (index + i < BufferSize())
            {
               index = Main::Playlist().Index(Buffer().Get(index + i));

               if (index >= 0)
               {
                  Debug("Calling delete on %s", Main::Playlist().Get(index)->URI().c_str());
                  client_.Delete(index);
               }
            }
         }
      }

      if ((scroll == true) && (posCount == 1))
      {
         Scroll(count);
      }
   }

   SelectWindow::DeleteLine(line, count, scroll);
}

void SongWindow::DeleteAllLines()
{
   DeleteLine(0, BufferSize(), false);
}

void SongWindow::Edit()
{
   if (CurrentLine() < BufferSize())
   {
      Mpc::Song * song(Buffer().Get(CurrentLine()));
      screen_.CreateSongInfoWindow(song);
   }
}

#ifdef LYRICS_SUPPORT
void SongWindow::Lyrics()
{
   if (CurrentLine() < BufferSize())
   {
      Mpc::Song * song(Buffer().Get(CurrentLine()));
      screen_.CreateSongLyricsWindow(song);
   }
}
#endif

void SongWindow::ScrollToFirstMatch(std::string const & input)
{
   for (uint32_t i = 0; i < BufferSize(); ++i)
   {
      Mpc::Song * song = Buffer().Get(i);
      std::string line = song->FormatString(settings_.Get(Setting::SongFormat));

      if (Algorithm::imatch(line, input, settings_.Get(Setting::IgnoreTheSort), settings_.Get(Setting::IgnoreCaseSort)) == true)
      {
         ScrollTo(i);
         break;
      }
   }
}


void SongWindow::Save(std::string const & name)
{
   if (Main::MpdLists().Index(Mpc::List(name)) == -1)
   {
      client_.CreatePlaylist(name);

      {
         Mpc::CommandList list(client_);

         for (unsigned int i = 0; i < BufferSize(); ++i)
         {
            client_.AddToNamedPlaylist(name, Buffer().Get(i));
         }
      }
   }
   else
   {
      ErrorString(ErrorNumber::PlaylistExists);
   }
}


void SongWindow::PrintBlankId() const
{
   WINDOW * window  = N_WINDOW();
   waddstr(window, " ");
}

void SongWindow::PrintId(uint32_t Id) const
{
   WINDOW * window  = N_WINDOW();

   waddstr(window, "[");

   if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(Id) == false))
   {
      wattron(window, COLOR_PAIR(settings_.colours.SongId));
   }

   wprintw(window, "%5d", Id + 1);

   if ((settings_.Get(Setting::ColourEnabled) == true) && (IsSelected(Id) == false))
   {
      wattroff(window, COLOR_PAIR(settings_.colours.SongId));
   }

   waddstr(window, "] ");
}


int32_t SongWindow::DetermineColour(uint32_t line) const
{
   uint32_t printLine = line + FirstLine();
   Mpc::Song * song   = (printLine < BufferSize()) ? Buffer().Get(printLine) : NULL;

   int32_t colour = settings_.colours.Song;

   if (song != NULL)
   {
      if ((song->URI() == clientState_.GetCurrentSongURI()))
      {
         colour = settings_.colours.CurrentSong;
      }
      else if (song->Reference() != 0)
      {
         colour = settings_.colours.FullAdd;
      }
      else if ((search_.LastSearchString() != "") && (settings_.Get(Setting::HighlightSearch) == true) &&
               (search_.HighlightSearch() == true))
      {
         Regex::RE const expression(".*" + search_.LastSearchString() + ".*", search_.LastSearchOptions());

         if (expression.CompleteMatch(song->FormatString(settings_.Get(Setting::SongFormat))))
         {
            colour = settings_.colours.SongMatch;
         }
      }
   }

   return colour;
}

uint32_t SongWindow::GetPositions(int64_t & pos1, int64_t & pos2) const
{
   pos1 = CurrentSelection().first;
   pos2 = CurrentSelection().second;

   if (pos2 < pos1)
   {
      pos2 = pos1;
      pos1 = CurrentSelection().second;
   }

   return (static_cast<uint32_t>(pos2 - pos1 + 1));
}

void SongWindow::Clear()
{
   ScrollTo(0);
   Buffer().Clear();
}

/* vim: set sw=3 ts=3: */
