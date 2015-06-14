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

   normal.cpp - normal mode input handling
   */

#include "normal.hpp"

#include "clientstate.hpp"
#include "mpdclient.hpp"
#include "buffer/library.hpp"
#include "buffer/playlist.hpp"
#include "window/debug.hpp"
#include "window/error.hpp"
#include "window/songwindow.hpp"
#include "mode/command.hpp"
#include "mode/inputmode.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#define ESCAPE_KEY 27

#if (NCURSES_MOUSE_VERSION <= 1)
#ifndef BUTTON5_PRESSED
#define BUTTON5_PRESSED BUTTON4_PRESSED << 8
#endif
#endif



using namespace Ui;

Normal::Normal(Main::Vimpc * vimpc, Ui::Screen & screen, Mpc::Client & client, Mpc::ClientState & clientState, Main::Settings & settings, Ui::Search & search) :
   Player           (screen, client, clientState, settings),
   window_          (NULL),
   actionCount_     (0),
   lastAction_      (""),
   lastActionCount_ (1),
   wasSpecificCount_(false),
   addMark_         (false),
   gotoMark_        (false),
   actionTable_     (),
   vimpc_           (vimpc),
   search_          (search),
   screen_          (screen),
   client_          (client),
   clientState_     (clientState),
   playlist_        (Main::Playlist()),
   settings_        (settings)
{
   // \todo display current count somewhere ?
   actionTable_["."]       = &Normal::RepeatLastAction;
   actionTable_["c"]       = &Normal::ClearScreen;

   // Player
   actionTable_["s"]       = &Normal::Stop;
   actionTable_["<BS>"]    = &Normal::Stop;
   actionTable_["<Space>"] = &Normal::PlayPause;

   actionTable_["C"]       = &Normal::Consume;
   actionTable_["T"]       = &Normal::Crossfade;
   actionTable_["R"]       = &Normal::Random;
   actionTable_["E"]       = &Normal::Repeat;
   actionTable_["S"]       = &Normal::Single;

   actionTable_["+"]       = &Normal::ChangeVolume<1>;
   actionTable_["-"]       = &Normal::ChangeVolume<-1>;

   // Console
   // \todo add an ex mode to console that just stays in command entry mode
   // This should really be implemented as Q as per vim (:he Ex-mode)
   //actionTable_['Q']       = &Normal::ExMode;

   // Skipping
   actionTable_["I"]       = &Normal::SeekTo<Player::Start>;
   actionTable_["^"]       = &Normal::SeekTo<Player::Start>;
   actionTable_[">"]       = &Normal::SkipSong<Player::Next>;
   actionTable_["<lt>"]    = &Normal::SkipSong<Player::Previous>;
   actionTable_["]"]       = &Normal::SkipArtist<Player::Next>;
   actionTable_["["]       = &Normal::SkipArtist<Player::Previous>;
   actionTable_["}"]       = &Normal::SkipAlbum<Player::Next>;
   actionTable_["{"]       = &Normal::SkipAlbum<Player::Previous>;

   // Selection
   actionTable_["H"]       = &Normal::Select<ScrollWindow::First>;
   actionTable_["M"]       = &Normal::Select<ScrollWindow::Middle>;
   actionTable_["L"]       = &Normal::Select<ScrollWindow::Last>;

   // Playlist
   actionTable_["d"]       = &Normal::Delete<Item::Single>;
   actionTable_["D"]       = &Normal::Delete<Item::All>;
   actionTable_["a"]       = &Normal::Add<Item::Single>;
   actionTable_["A"]       = &Normal::Add<Item::All>;
   actionTable_["x"]       = &Normal::Crop<Mpc::Song::Single>;
   actionTable_["X"]       = &Normal::Crop<Mpc::Song::All>;
   actionTable_["<Del>"]   = &Normal::Delete<Item::Single>;
   actionTable_["p"]       = &Normal::PasteBuffer<Screen::Down>;
   actionTable_["P"]       = &Normal::PasteBuffer<Screen::Up>;

   // Navigation
   actionTable_["<Nop>"]   = &Normal::DoNothing;
   actionTable_["<Esc>"]   = &Normal::Escape;
   actionTable_["l"]       = &Normal::Right;
   actionTable_["h"]       = &Normal::Left;
   actionTable_["\n"]      = &Normal::Confirm;
   actionTable_["<Return>"]= &Normal::Confirm;
   actionTable_["<C-J>"]   = &Normal::Confirm;
   actionTable_["<Enter>"] = &Normal::Confirm;
   actionTable_["<CR>"]    = &Normal::Confirm;
   actionTable_["<LeftMouse>"] = &Normal::Click;
   actionTable_["<2-LeftMouse>"] = &Normal::Confirm;
   actionTable_["W"]       = &Normal::SetActiveAndVisible<Ui::Screen::WindowSelect>;

   // Searching
   actionTable_["N"]       = &Normal::SearchResult<Search::Previous>;
   actionTable_["n"]       = &Normal::SearchResult<Search::Next>;

   // Scrolling
   actionTable_["k"]       = &Normal::Scroll<Screen::Line, Screen::Up>;
   actionTable_["j"]       = &Normal::Scroll<Screen::Line, Screen::Down>;
   actionTable_["<PageUp>"]   = &Normal::Scroll<Screen::Page, Screen::Up>;
   actionTable_["<PageDown>"] = &Normal::Scroll<Screen::Page, Screen::Down>;
   actionTable_["<C-U>"]   = &Normal::Scroll<Screen::Page, Screen::Up>;
   actionTable_["<C-D>"]   = &Normal::Scroll<Screen::Page, Screen::Down>;
   actionTable_["<C-B>"]   = &Normal::Scroll<Screen::FullPage, Screen::Up>;
   actionTable_["<C-F>"]   = &Normal::Scroll<Screen::FullPage, Screen::Down>;
   actionTable_["<C-Y>"]   = &Normal::Align<Screen::Up>;
   actionTable_["<C-E>"]   = &Normal::Align<Screen::Down>;
   actionTable_["<Home>"]  = &Normal::ScrollTo<Screen::Top>;
   actionTable_["f"]       = &Normal::ScrollToCurrent<1>;
   actionTable_["F"]       = &Normal::ScrollToCurrent<-1>;
   actionTable_["<End>"]   = &Normal::ScrollTo<Screen::Bottom>;
   actionTable_["G"]       = &Normal::ScrollTo<Screen::Specific, Screen::Bottom>;
   actionTable_["%"]       = &Normal::ScrollTo<Screen::Random>;

   actionTable_["<ScrollWheelUp>"]   = &Normal::Scroll<-6>;
   actionTable_["<ScrollWheelDown>"] = &Normal::Scroll<6>;

	// Handled by queue input handler now
   //actionTable_["<C-Z>"]   = &Normal::SendSignal<SIGTSTP>;
   //actionTable_["<C-C>"]   = &Normal::SendSignal<SIGINT>;

   // Editing
   actionTable_["<C-A>"]   = &Normal::Move<Relative, 1>;
   actionTable_["<C-X>"]   = &Normal::Move<Relative, -1>;

   //
   actionTable_["<Left>"]  = actionTable_["h"];
   actionTable_["<Right>"] = actionTable_["l"];
   actionTable_["<Down>"]  = actionTable_["j"];
   actionTable_["<Up>"]    = actionTable_["k"];

   //
   actionTable_["q"]       = &Normal::Close;
   actionTable_["e"]       = &Normal::Edit;
#ifdef LYRICS_SUPPORT
   actionTable_["y"]       = &Normal::Lyrics;
#endif
   actionTable_["v"]       = &Normal::Visual;
   actionTable_["V"]       = &Normal::Visual;
   actionTable_["<C-V>"]   = &Normal::Visual;
   actionTable_["o"]       = &Normal::SwitchVisualEnd;

   // Library
   actionTable_["zo"]      = &Normal::Expand;
   actionTable_["zc"]      = &Normal::Collapse;

   // Jumping
   actionTable_["gg"]      = &Normal::ScrollTo<Screen::Specific, Screen::Top>;
   actionTable_["gf"]      = &Normal::ScrollToCurrent<1>;
   actionTable_["gF"]      = &Normal::ScrollToCurrent<-1>;
   actionTable_["gp"]      = &Normal::ScrollToPlaylistSong<Search::Next>;
   actionTable_["gP"]      = &Normal::ScrollToPlaylistSong<Search::Previous>;
   actionTable_["gt"]      = &Normal::SetActiveWindow<Screen::Next, 0>;
   actionTable_["gT"]      = &Normal::SetActiveWindow<Screen::Previous, 0>;
   actionTable_["gv"]      = &Normal::ResetSelection;
   actionTable_["gm"]      = &Normal::Move<Absolute, 0>;

   // Marks
   actionTable_["m"]       = &Normal::NextAddMark;
   actionTable_["g`"]      = &Normal::NextGotoMark;
   actionTable_["g'"]      = &Normal::NextGotoMark;
   actionTable_["'"]       = &Normal::NextGotoMark;
   actionTable_["`"]       = &Normal::NextGotoMark;

   // Align the text to a location on the screen
   actionTable_["z."]        = &Normal::AlignTo<Screen::Centre>;
   actionTable_["zz"]        = &Normal::AlignTo<Screen::Centre>;
   actionTable_["z<Enter>"]  = &Normal::AlignTo<Screen::Top>;
   actionTable_["z<Return>"] = &Normal::AlignTo<Screen::Top>;
   actionTable_["zt"]        = &Normal::AlignTo<Screen::Top>;
   actionTable_["z-"]        = &Normal::AlignTo<Screen::Bottom>;
   actionTable_["zb"]        = &Normal::AlignTo<Screen::Bottom>;

   // Support for Z{Z,Q}
   actionTable_["ZZ"]        = &Normal::Quit;
   actionTable_["ZQ"]        = &Normal::QuitAll;

   // Alt key combos
   actionTable_["<A-1>"]     = &Normal::SetActiveWindow<Screen::Absolute, 0>;
   actionTable_["<A-2>"]     = &Normal::SetActiveWindow<Screen::Absolute, 1>;
   actionTable_["<A-3>"]     = &Normal::SetActiveWindow<Screen::Absolute, 2>;
   actionTable_["<A-4>"]     = &Normal::SetActiveWindow<Screen::Absolute, 3>;
   actionTable_["<A-5>"]     = &Normal::SetActiveWindow<Screen::Absolute, 4>;
   actionTable_["<A-6>"]     = &Normal::SetActiveWindow<Screen::Absolute, 5>;
   actionTable_["<A-7>"]     = &Normal::SetActiveWindow<Screen::Absolute, 6>;
   actionTable_["<A-8>"]     = &Normal::SetActiveWindow<Screen::Absolute, 7>;
   actionTable_["<A-9>"]     = &Normal::SetActiveWindow<Screen::Absolute, 8>;

   window_ = screen.CreateModeWindow();
}

Normal::~Normal()
{
   screen_.DeleteModeWindow(window_);
   window_ = NULL;
}

void Normal::Initialise(int input)
{
   actionCount_ = 0;
   Refresh();
}

void Normal::Finalise(int input)
{
   input_ = "";
   screen_.PrintModeWindow(window_);
}

void Normal::Refresh()
{
   DisplayModeLine();
   screen_.PrintModeWindow(window_);
}

bool Normal::Handle(int input)
{
   bool result = true;

   if (gotoMark_ == true)
   {
      GotoMark(InputCharToString(input));
   }
   else if (addMark_ == true)
   {
      AddMark(InputCharToString(input));
   }
   else if ((input >= '0') && (input <= '9') && ((input & (1 << 31)) == 0))
   {
      uint64_t const newActionCount = ((static_cast<uint64_t>(actionCount_) * 10) + (input - '0'));

      if (newActionCount <= std::numeric_limits<uint32_t>::max())
      {
         actionCount_ = newActionCount;
      }
   }
   else
   {
      wasSpecificCount_ = (actionCount_ != 0);

      uint32_t count = (actionCount_ > 0) ? actionCount_ : 1;

      // Convert character input into vimpc strings using conversion table
      input_ += InputCharToString(input);

      bool const complete = Handle(input_, count);

      if (complete == true)
      {
         if (input_ != ".")
         {
            lastAction_      = input_;
            lastActionCount_ = count;
         }

         actionCount_ = 0;
         input_       = "";
      }
   }

   return result;
}


bool Normal::CausesModeToStart(int input) const
{
   return ((input == '\n') || (input == ESCAPE_KEY));
}

bool Normal::CausesModeToEnd(int input) const
{
   return false;
}


void Normal::Execute(std::string const & input)
{
   std::vector<KeyMapItem> KeyMap;

   bool const valid = CreateKeyMap(input, KeyMap);

   if (valid == true)
   {
      RunKeyMap(KeyMap, 1);
   }
   else
   {
      ErrorString(ErrorNumber::CouldNotParseKeys);
   }
}


void Normal::Map(std::string key, std::string mapping)
{
   std::vector<KeyMapItem> KeyMap;

   // Ensure that the key is specified in a uniform manner
   key = PerformMapSubtitutions(key);

   bool const valid = CreateKeyMap(mapping, KeyMap);

   if (valid == true)
   {
      mapTable_[key] = KeyMap;
      mapNames_[key] = mapping;
   }
   else
   {
      ErrorString(ErrorNumber::CouldNotMapKeys);
   }
}

void Normal::WindowMap(int window, std::string key, std::string mapping, bool store)
{
   std::vector<KeyMapItem> KeyMap;

   // Ensure that the key is specified in a uniform manner
   key = PerformMapSubtitutions(key);

   bool const valid = CreateKeyMap(mapping, KeyMap, window);

   if (valid == true)
   {
      windowMap_[window][key] = KeyMap;

      if (store == true)
      {
         windowMapNames_[window][key] = mapping;
      }
   }
   else
   {
      ErrorString(ErrorNumber::CouldNotMapKeys);
   }
}

std::vector<Normal::KeyMapItem> Normal::CreateKeyMap(std::string const & mapping)
{
   std::vector<KeyMapItem> KeyMap;
   CreateKeyMap(mapping, KeyMap);
   return KeyMap;
}

bool Normal::CreateKeyMap(std::string const & mapping, std::vector<KeyMapItem> & KeyMap, int window)
{
   bool error = false;
   int  count = 0;

   for (uint32_t i = 0; i < mapping.length(); )
   {
      KeyMapItem item;
      char const next = mapping.at(i);

      if ((next >= '0') && (next <= '9'))
      {
         count *= 10;
         count += next - '0';
         ++i;
      }
      else
      {
         std::string input;
         std::string toMap = mapping.substr(i);

         bool inWindowMap = false;

         // If this is a normal mode mapping ensure that the keys being mapped
         // are specified in the correct way, i.e replace <sPace> with <Space>
         // but do not do this if we are in an input mode
         if (vimpc_->ModeAfterInput(Main::Vimpc::Normal, toMap.at(0)) == Main::Vimpc::Normal)
         {
            toMap = PerformMapSubtitutions(toMap);
         }

         if (window >= 0)
         {
            inWindowMap = CheckTableForInput(windowMap_[window], toMap, input);
         }

         bool const found = CheckTableForInput(mapTable_, toMap, input);

         if ((found == false) && (inWindowMap == false))
         {
            error = !(CheckTableForInput(actionTable_, toMap, input));

            if (error == false)
            {
               item.action_ = actionTable_[input];
               item.count_  = count;
               KeyMap.push_back(item);
            }
            else if (vimpc_->RequiresModeChange(Main::Vimpc::Normal, toMap.at(0)) == true)
            {
               error = false;
               input = Ui::InputMode::SplitStringAtTerminator(toMap);

               item.mode_   = vimpc_->ModeAfterInput(Main::Vimpc::Normal, toMap.at(0));
               item.input_  = input;
               item.count_  = count;
               KeyMap.push_back(item);
            }
         }
         else
         {
            std::vector<KeyMapItem> KeyMapVector;

            if (inWindowMap == true)
            {
               KeyMapVector = windowMap_[screen_.GetActiveWindow()][input];
            }
            else
            {
               KeyMapVector = mapTable_[input];
            }

            for (auto newitem : KeyMapVector)
            {
               // \todo If there is a count before this bunch of stuff is mapped that won't work
               KeyMap.push_back(newitem);
            }
         }

         if (error == true)
         {
            break;
         }

         i += input.length();
         count = 0;
      }
   }

   return !error;
}

void Normal::Unmap(std::string key)
{
   mapTable_.erase(key);
   mapNames_.erase(key);
}

void Normal::WindowUnmap(int window, std::string key)
{
   windowMap_[window].erase(key);
   windowMapNames_[window].erase(key);
}

Ui::Normal::MapNameTable Normal::Mappings()
{
   return mapNames_;
}

Ui::Normal::MapNameTable Normal::WindowMappings(int window)
{
   return windowMapNames_[window];
}

template <typename T>
bool Normal::CheckTableForInput(T table, std::string const & toMap, std::string & result)
{
   uint32_t max = 0;

   for (auto entry : table)
   {
      if ((entry.first == toMap.substr(0, entry.first.length())) && (entry.first.length() >= max))
      {
         if (max < entry.first.length())
         {
            result = entry.first;
            max    = entry.first.length();
         }
      }
   }

   return (max != 0);
}

bool Normal::Handle(std::string input, int count)
{
   bool inMap    = false;
   bool complete = true;

   for (auto entry : windowMap_[screen_.GetActiveWindow()])
   {
      if (entry.first.substr(0, input.size()) == input)
      {
         complete = (entry.first == input) && complete;
         inMap    = true;
      }
   }

   if (inMap == false)
   {
      for (auto entry : mapTable_)
      {
         if (entry.first.substr(0, input.size()) == input)
         {
            complete = (entry.first == input) && complete;
            inMap    = true;
         }
      }
   }

   if ((inMap == true) && (complete == true))
   {
      HandleMap(input, count);
   }
   else if (inMap == false)
   {
      ptrToMember actionFunc = NULL;

      for (auto action : actionTable_)
      {
         if (action.first.substr(0, input.size()) == input)
         {
            complete   = (action.first == input) && complete;
            actionFunc = action.second;
         }
      }

      if ((complete == true) && (actionFunc != NULL))
      {
         Debug("Executing normal input %d%s", count, input.c_str());
         (*this.*actionFunc)(count);
      }
   }

   return complete;
}

void Normal::HandleMap(std::string input, int count)
{
   bool complete = false;

   for (int i = 0; ((i < count) && (complete == false)); ++i)
   {
      std::vector<KeyMapItem> KeyMap;

      if (windowMap_[screen_.GetActiveWindow()].find(input) != windowMap_[screen_.GetActiveWindow()].end())
      {
         KeyMap = windowMap_[screen_.GetActiveWindow()][input];
      }
      else
      {
         KeyMap = mapTable_[input];
      }

      complete = RunKeyMap(KeyMap, count);
   }
}

bool Normal::RunKeyMap(std::vector<KeyMapItem> const & KeyMap, int count)
{
   bool complete      = false;
   bool specificCount = wasSpecificCount_;

   for (auto item : KeyMap)
   {
      uint32_t param = (item.count_ > 0) ? item.count_ : 1;

      if (wasSpecificCount_ == false)
      {
         wasSpecificCount_ = (item.count_ > 0);
      }

      // If we are mapping to a single key
      if (KeyMap.size() == 1)
      {
         // Pass in the specified count, rather than looping count times
         // if there is only one key mapped and no count mapped
         if (item.count_ == 0)
         {
            param = count;
            complete = true;
         }
      }

      if ((item.mode_ == Main::Vimpc::Normal) && (item.action_ != NULL))
      {
         (*this.*(item.action_))(param);
      }
      else
      {
         vimpc_->ChangeMode(item.input_.at(0), item.input_.substr(1));
      }

      wasSpecificCount_ = false;
   }

   wasSpecificCount_ = specificCount;
   return complete;
}

std::string Normal::PerformMapSubtitutions(std::string input) const
{
   static std::map<std::string, std::string> conversionTable;
   static char key[32];
   static char value[32];

   // \TODO these conversion table things need tidying up
   // and properly use a variable for the strings since they are shared
   if (conversionTable.size() == 0)
   {
      // Add F1 - F12  into the conversion table
      for (int i = 0; i <= 12; ++i)
      {
         sprintf(key, "<f%d>", i);
         sprintf(value, "<F%d>", i);
         conversionTable[std::string(key)] = std::string(value);
      }

      // Alt key combinations
      for (char i = 0; i < 127; ++i)
      {
         sprintf(key, "<a-%c>", i);
         sprintf(value, "<A-%c>", i);
         conversionTable[std::string(key)] = std::string(value);
      }

      // Ctrl key combinations
      for (char i = 0; i < 127; ++i)
      {
         sprintf(key, "<c-%c>", i);
         sprintf(value, "<C-%c>", toupper(i));
         conversionTable[std::string(key)] = std::string(value);
      }

      conversionTable["<nop>"]       = "<Nop>";
      conversionTable["<esc>"]       = "<Esc>";
      conversionTable["<pageup>"]    = "<PageUp>";
      conversionTable["<pagedown>"]  = "<PageDown>";
      conversionTable["<home>"]      = "<Home>";
      conversionTable["<end>"]       = "<End>";
      conversionTable["<left>"]      = "<Left>";
      conversionTable["<right>"]     = "<Right>";
      conversionTable["<down>"]      = "<Down>";
      conversionTable["<up>"]        = "<Up>";
      conversionTable["<del>"]       = "<Del>";
      conversionTable["<bs>"]        = "<BS>";
      conversionTable["<space>"]     = "<Space>";
      conversionTable["<enter>"]     = "<Enter>";
      conversionTable["<return>"]    = "<Return>";
      conversionTable["<lt>"]        = "<lt>";
      conversionTable["<tab>"]       = "<Tab>";
      conversionTable["<sc>"]        = "<sc>";
      conversionTable["<cr>"]        = "<CR>";
      conversionTable["<c-h>"]       = "<BS>";

   }

   Regex::RE const regex("^([^<]*(<?[^>]*>?)).*$");

   std::string inputReplace(input);
   std::string mapping, matchString;

   // Correct all special key names to be in a uniform, consistent and pretty way
   // i.e this replaces <sPaCe> with <Space>, etc
   while ((inputReplace.size() > 0) && (regex.Capture(inputReplace, &matchString, &mapping) == true))
   {
      inputReplace = inputReplace.substr(matchString.size());

      Regex::RE check(mapping);

      // Special case handling for alt keys, as A-y and A-Y are indeed quite different
      if ((mapping.length() > 3) &&
          ((mapping.substr(0, 3) == "<A-") || (mapping.substr(0, 3) == "<a-")))
      {
         mapping.replace(0, 3, "<A-");
      }
      else
      {
         std::transform(mapping.begin(), mapping.end(), mapping.begin(), ::tolower);

         if (conversionTable.find(mapping) != conversionTable.end())
         {
            mapping = conversionTable[mapping];
         }
      }

      check.Replace(mapping, input);
   }

   return input;
}

std::string Normal::InputCharToString(int input) const
{
   static std::map<int, std::string> conversionTable;
   static char key[32];

   if (conversionTable.empty() == true)
   {
      conversionTable[ESCAPE_KEY]    = "<Esc>";
      conversionTable[KEY_PPAGE]     = "<PageUp>";
      conversionTable[KEY_NPAGE]     = "<PageDown>";
      conversionTable[KEY_HOME]      = "<Home>";
      conversionTable[KEY_END]       = "<End>";
      conversionTable[KEY_LEFT]      = "<Left>";
      conversionTable[KEY_RIGHT]     = "<Right>";
      conversionTable[KEY_DOWN]      = "<Down>";
      conversionTable[KEY_UP]        = "<Up>";
      conversionTable[KEY_DC]        = "<Del>";
      conversionTable[KEY_BACKSPACE] = "<BS>";
      conversionTable[0x20]          = "<Space>";
      conversionTable[0x7F]          = "<BS>";
      conversionTable[KEY_ENTER]     = "<Enter>";
      conversionTable['\n']          = "<Return>";
      conversionTable['<']           = "<lt>";
      conversionTable['\t']          = "<Tab>";
      conversionTable[';']           = "<sc>";

      // Add F1 - F12  into the conversion table
      for (int i = 0; i <= 12; ++i)
      {
         sprintf(key, "<F%d>", i);
         conversionTable[KEY_F(i)] = std::string(key);
      }
   }

   std::string result = "";

#ifdef HAVE_MOUSE_SUPPORT
   if (input == KEY_MOUSE)
   {
      if (settings_.Get(::Setting::Mouse) == true)
      {
         result = MouseInputToString();
      }
   }
   else
   {
#endif
      auto it = conversionTable.find(input);

      if (it != conversionTable.end())
      {
         result = it->second;
      }
      else
      {
         result += static_cast<char>(input);

         // Alt key combinations
         if ((input & (1 << 31)) != 0)
         {
            sprintf(key, "<A-%c>", char (input & 0x7FFFFFFF));
            result = std::string(key);
         }
         // Ctrl key combinations
         else if ((input <= 27) && (input >= 1))
         {
            sprintf(key, "<C-%c>", char ('A' + input - 1));
            result = std::string(key);
         }
      }
#ifdef HAVE_MOUSE_SUPPORT
   }
#endif

   return result;
}

std::string Normal::MouseInputToString() const
{
#ifdef HAVE_MOUSE_SUPPORT
   //! \TODO this seems to scroll quite slowly and not properly at all
   static std::map<uint32_t, std::string> conversionTable;

   if (conversionTable.empty() == true)
   {
#if (NCURSES_MOUSE_VERSION <= 1)
      conversionTable[BUTTON5_PRESSED]        = "<ScrollWheelDown>";
#endif
      conversionTable[BUTTON2_PRESSED]        = "<ScrollWheelDown>";
      conversionTable[BUTTON4_PRESSED]        = "<ScrollWheelUp>";
      conversionTable[BUTTON1_CLICKED]        = "<LeftMouse>";
      conversionTable[BUTTON1_DOUBLE_CLICKED] = "<2-LeftMouse>";
      conversionTable[BUTTON3_CLICKED]        = "<RightMouse>";
      conversionTable[BUTTON3_DOUBLE_CLICKED] = "<2-RightMouse>";
   }

   MEVENT event = screen_.LastMouseEvent();

   for (auto conv : conversionTable)
   {
      if ((conv.first & event.bstate) == conv.first)
      {
         return conv.second;
      }
   }

#endif
   return "";
}


void Normal::ClearScreen(uint32_t count)
{
   Player::ClearScreen();
}

void Normal::PlayPause(uint32_t count)
{
   if (clientState_.CurrentState() == "Stopped")
   {
     client_.Play(0);
   }
   else
   {
      Player::Pause();
   }
}

void Normal::Pause(uint32_t count)
{
   Player::Pause();
}

void Normal::Stop(uint32_t count)
{
   Player::Stop();
}


void Normal::Consume(uint32_t count)
{
   Player::ToggleConsume();
}

void Normal::Crossfade(uint32_t count)
{
   Player::ToggleCrossfade();
}

void Normal::Random(uint32_t count)
{
   Player::ToggleRandom();
}

void Normal::Repeat(uint32_t count)
{
   Player::ToggleRepeat();
}

void Normal::Single(uint32_t count)
{
   Player::ToggleSingle();
}


template <int Delta>
void Normal::ChangeVolume(uint32_t count)
{
   client_.DeltaVolume(count * Delta);
}

template <Ui::Player::Location LOCATION>
void Normal::SeekTo(uint32_t count)
{
   if (LOCATION == Player::Start)
   {
      client_.SeekTo(0);
   }
   else if (LOCATION == Player::End)
   {
      Player::SkipSong(Player::Next, count);
   }
}

void Normal::Left(uint32_t count)
{
   screen_.ActiveWindow().Left(*this, count);
}

void Normal::Right(uint32_t count)
{
   screen_.ActiveWindow().Right(*this, count);
}

void Normal::Click(uint32_t count)
{
   screen_.ActiveWindow().Click();
}

void Normal::Confirm(uint32_t count)
{
   static WindowActionTable confirmTable;

   if (confirmTable.size() == 0)
   {
      // \todo move lots of common functions into the "player"
      // so that i can share stuff from command
      confirmTable[Ui::Screen::Outputs]      = &Normal::ToggleOutput<Item::Single>;
      confirmTable[Ui::Screen::Playlist]     = &Normal::PlaySelected;
   }

   WindowActionTable::const_iterator it =
      confirmTable.find(static_cast<Ui::Screen::MainWindow>(screen_.GetActiveWindow()));

   if (it != confirmTable.end())
   {
      ptrToMember actionFunc = it->second;
      (*this.*actionFunc)(count);
   }
   else
   {
      screen_.ActiveWindow().Confirm();
   }
}

void Normal::Escape(uint32_t count)
{
   input_       = "";
   actionCount_ = 0;
   screen_.ActiveWindow().Escape();
}

void Normal::RepeatLastAction(uint32_t count)
{
   if (wasSpecificCount_ == false)
   {
      count = lastActionCount_;
   }

   if (lastAction_ != "")
   {
      Handle(lastAction_, count);
   }
}

void Normal::Expand(uint32_t count)
{
   // \TODO this is pretty dodgy is doesn't check the proper windows
   if (screen_.ActiveWindow().CurrentLine() < Main::Library().Size())
   {
      Main::Library().Expand(screen_.ActiveWindow().CurrentLine());
   }
}

void Normal::Collapse(uint32_t count)
{
   // \TODO this is pretty dodgy is doesn't check the proper windows
   if (screen_.ActiveWindow().CurrentLine() < Main::Library().Size())
   {
      Main::Library().Collapse(screen_.ActiveWindow().CurrentLine());
   }
}


void Normal::Close(uint32_t count)
{
   screen_.SetVisible(screen_.GetActiveWindow(), false);
}

void Normal::Edit(uint32_t count)
{
   screen_.ActiveWindow().Edit();
}

#ifdef LYRICS_SUPPORT
void Normal::Lyrics(uint32_t count)
{
   screen_.ActiveWindow().Lyrics();
}
#endif

void Normal::Visual(uint32_t count)
{
   screen_.ActiveWindow().Visual();
}

void Normal::SwitchVisualEnd(uint32_t count)
{
   screen_.ActiveWindow().SwitchVisualEnd();
}


void Normal::PlaySelected(uint32_t count)
{
   int32_t song = screen_.GetSelected(Ui::Screen::Playlist);

   if (song >= 0)
   {
      Player::Play(song);
   }
}

template <Item::Collection COLLECTION>
void Normal::ToggleOutput(uint32_t count)
{
   if (COLLECTION == Item::Single)
   {
      int32_t output = screen_.GetSelected(Ui::Screen::Outputs);

      for (uint32_t i = 0; i < count; ++i)
      {
         Player::ToggleOutput(output + i);
      }
   }
   else
   {
      Player::ToggleOutput(COLLECTION);
   }
}

template <Item::Collection COLLECTION, bool ENABLE>
void Normal::SetOutput(uint32_t count)
{
   if (COLLECTION == Item::Single)
   {
      int32_t output = screen_.GetSelected(Ui::Screen::Outputs);

      for (uint32_t i = 0; i < count; ++i)
      {
         Player::SetOutput(output + i, ENABLE);
      }
   }
   else
   {
      Player::SetOutput(COLLECTION, ENABLE);
   }
}


template <Item::Collection COLLECTION>
void Normal::Add(uint32_t count)
{
   static WindowActionTable confirmTable;

   if (clientState_.Connected())
   {
      if (confirmTable.size() == 0)
      {
         confirmTable[Ui::Screen::Outputs] = &Normal::SetOutput<COLLECTION, true>;
      }

      WindowActionTable::const_iterator it =
         confirmTable.find(static_cast<Ui::Screen::MainWindow>(screen_.GetActiveWindow()));

      if (it != confirmTable.end())
      {
         ptrToMember actionFunc = it->second;
         (*this.*actionFunc)(count);
      }
      else
      {
         if (COLLECTION == Item::All)
         {
            screen_.ActiveWindow().AddAllLines();
         }
         else if (COLLECTION == Item::Single)
         {
            screen_.ActiveWindow().AddLine(screen_.ActiveWindow().CurrentLine(), count, settings_.Get(Setting::ScrollOnAdd));
         }

         client_.AddComplete();
      }
   }
}

template <Item::Collection COLLECTION>
void Normal::Delete(uint32_t count)
{
   static WindowActionTable confirmTable;

   if (clientState_.Connected())
   {
      if (confirmTable.size() == 0)
      {
         confirmTable[Ui::Screen::Outputs]  = &Normal::SetOutput<COLLECTION, false>;
      }

      WindowActionTable::const_iterator it =
         confirmTable.find(static_cast<Ui::Screen::MainWindow>(screen_.GetActiveWindow()));

      if (it != confirmTable.end())
      {
         ptrToMember actionFunc = it->second;
         (*this.*actionFunc)(count);
      }
      else
      {
         if (COLLECTION == Item::All)
         {
            screen_.ActiveWindow().DeleteAllLines();
         }
         else if (COLLECTION == Item::Single)
         {
            screen_.ActiveWindow().DeleteLine(screen_.ActiveWindow().CurrentLine(), count, settings_.Get(Setting::ScrollOnDelete));
         }
      }
   }
}

template <Mpc::Song::SongCollection COLLECTION>
void Normal::Crop(uint32_t count)
{
   if (COLLECTION == Mpc::Song::All)
   {
      screen_.ActiveWindow().CropAllLines();
   }
   else if (COLLECTION == Mpc::Song::Single)
   {
      screen_.ActiveWindow().CropLine(screen_.ActiveWindow().CurrentLine(), count);
   }
}

template <Screen::Direction DIRECTION>
void Normal::PasteBuffer(uint32_t count)
{
   uint32_t direction = Main::Playlist().Size() == 0 ? 0 : DIRECTION;

   Mpc::CommandList list(client_);

   if (screen_.GetActiveWindow() == Screen::Playlist)
   {
      for (uint32_t i = 0; i < count; ++i)
      {
         for (uint32_t j = 0; j < Main::PlaylistPasteBuffer().Size(); ++j)
         {
            client_.Add(*Main::PlaylistPasteBuffer().Get(j),
                        screen_.ActiveWindow().CurrentLine() + j + direction);
         }
      }

      screen_.Scroll(Main::PlaylistPasteBuffer().Size() * count * direction);
   }
}


//Implementation of selecting functions
template <ScrollWindow::Position POSITION>
void Normal::Select(uint32_t count)
{
   screen_.Select(POSITION, count);
}


//Implementation of searching functions
template <Ui::Search::Skip SKIP>
void Normal::SearchResult(uint32_t count)
{
   if (screen_.GetActiveWindow() != Screen::DebugConsole)
   {
      search_.SearchResult(SKIP, count);
   }
}


//Implementation of skipping functions
template <Ui::Player::Skip SKIP>
void Normal::SkipSong(uint32_t count)
{
   Player::SkipSong(SKIP, count);
}

template <Ui::Player::Skip SKIP>
void Normal::SkipAlbum(uint32_t count)
{
   Player::SkipAlbum(SKIP, count);
}

template <Ui::Player::Skip SKIP>
void Normal::SkipArtist(uint32_t count)
{
   Player::SkipArtist(SKIP, count);
}


// Implementation of scrolling functions
template <int8_t OFFSET>
void Normal::ScrollToCurrent(uint32_t line)
{
   screen_.ScrollTo(Screen::Current, (wasSpecificCount_ == true) ? line * OFFSET : 0);
}

template <int8_t OFFSET>
void Normal::Scroll(uint32_t count)
{
   int32_t scroll = OFFSET * count;
   screen_.Scroll(scroll);
}

template <Screen::Size SIZE, Screen::Direction DIRECTION>
void Normal::Scroll(uint32_t count)
{
   screen_.Scroll(SIZE, DIRECTION, count);
}

template <Screen::Location LOCATION>
void Normal::ScrollTo(uint32_t line)
{
   screen_.ScrollTo(LOCATION);
}

template <Screen::Location SPECIFIC, Screen::Location ENDLOCATION>
void Normal::ScrollTo(uint32_t line)
{
   if ((SPECIFIC == Screen::Specific) && (wasSpecificCount_ == false))
   {
      ScrollTo<ENDLOCATION>(line);
   }
   else
   {
      screen_.ScrollTo(SPECIFIC, line);
   }
}


template <Search::Skip SKIP>
void Normal::ScrollToPlaylistSong(uint32_t count)
{
   if (SKIP == Search::Previous)
   {
      screen_.ScrollTo(screen_.ActiveWindow().Playlist(-1 * count));
   }
   else
   {
      screen_.ScrollTo(screen_.ActiveWindow().Playlist(count));
   }
}


void Normal::NextGotoMark(uint32_t count)
{
   gotoMark_ = true;
}

void Normal::NextAddMark(uint32_t count)
{
   addMark_ = true;
}

void Normal::AddMark(std::string const & input)
{
   addMark_ = false;
   markTable_[input] = std::pair<uint32_t, uint32_t>(screen_.GetActiveWindow(), screen_.ActiveWindow().CurrentLine());
}

void Normal::GotoMark(std::string const & input)
{
   gotoMark_ = false;

   // Marks A-Z jump to the first line starting
   // with that letter
   if ((input[0] >= 'a') && (input[0] <= 'z'))
   {
      screen_.ScrollToAZ(input);
   }
   else if (markTable_.find(input) != markTable_.end())
   {
      MarkTable::const_iterator it = markTable_.find(input);
      screen_.SetActiveAndVisible(static_cast<Screen::MainWindow>(it->second.first));
      screen_.ScrollTo(it->second.second);
   }
   else
   {
      ErrorString(ErrorNumber::NoSuchMark);
   }
}


template <Screen::Direction DIRECTION>
void Normal::Align(uint32_t count)
{
   screen_.Align(DIRECTION, count);
}

template <Screen::Location LOCATION>
void Normal::AlignTo(uint32_t line)
{
   if (wasSpecificCount_ == false)
   {
      line = 0;
   }

   screen_.AlignTo(LOCATION, line);
}

//Implementation of window change function
template <Ui::Screen::MainWindow MAINWINDOW>
void Normal::SetActiveAndVisible(uint32_t count)
{
   screen_.SetActiveAndVisible(static_cast<int32_t>(MAINWINDOW));
}

// Implementation of window functions
template <Screen::Skip SKIP, uint32_t OFFSET>
void Normal::SetActiveWindow(uint32_t count)
{
   if (SKIP == Screen::Absolute)
   {
      screen_.SetActiveWindow(static_cast<Screen::MainWindow>(OFFSET));
   }
   else if ((SKIP == Screen::Next) && (wasSpecificCount_ == true))
   {
      screen_.SetActiveWindow(static_cast<Screen::MainWindow>(count - 1));
   }
   else if ((SKIP == Screen::Previous) && (wasSpecificCount_ == true))
   {
      count = (count % screen_.VisibleWindows());

      for (uint32_t i = 0; i < count; ++i)
      {
         screen_.SetActiveWindow(SKIP);
      }
   }
   else
   {
      screen_.SetActiveWindow(SKIP);
   }
}

void Normal::ResetSelection(uint32_t count)
{
   screen_.ActiveWindow().ResetSelection();
}

// Proxy for quit commands
void Normal::Quit(uint32_t count)
{
   QuitAll(0);
}

void Normal::QuitAll(uint32_t count)
{
   if (settings_.Get(Setting::StopOnQuit) == true)
   {
      Player::Stop();
   }

   Player::Quit();
}

// Implementation of editting functions
template <Normal::move_t MOVE, int8_t OFFSET>
void Normal::Move(uint32_t count)
{
   if (screen_.GetActiveWindow() == Screen::Playlist)
   {
      uint32_t const currentLine = screen_.ActiveWindow().CurrentLine();
      int32_t position = 0;

      if (MOVE == Relative)
      {
         position = currentLine + (count * OFFSET);
      }
      else
      {
         position = count - 1;
      }

      if (position >= static_cast<int32_t>(screen_.ActiveWindow().BufferSize()))
      {
         position = screen_.ActiveWindow().BufferSize() - 1;
      }
      else if (position <= 0)
      {
         position = 0;
      }

      if (currentLine < Main::Playlist().Size())
      {
         client_.Move(currentLine, position);

         Mpc::Song * song = Main::Playlist().Get(currentLine);
         Main::Playlist().Remove(currentLine, 1);
         Main::Playlist().Add(song, position);
         screen_.ActiveWindow().ScrollTo(position);
         screen_.Update();
      }
   }
}


template <int SIGNAL>
void Normal::SendSignal(uint32_t count)
{
   kill(getpid(), SIGNAL);
}


void Normal::DisplayModeLine()
{
   std::string const state(StateString());
   std::string const scrolls(ScrollString());

   int32_t const WhiteSpaceLength = screen_.MaxColumns() - (state.size()) - (scrolls.size() - 1);

   std::string blankLine("");

   if (WhiteSpaceLength > 0)
   {
      blankLine = std::string(WhiteSpaceLength, ' ');
   }

   window_->SetLine("%s%s%s", state.c_str(), blankLine.c_str(), scrolls.c_str());
}

std::string Normal::ScrollString()
{
   std::ostringstream scrollStream;

   if (screen_.ActiveWindow().BufferSize() > 0)
   {
      float currentScroll = ((screen_.ActiveWindow().CurrentLine())/(static_cast<float>(screen_.ActiveWindow().BufferSize()) - 2));
      currentScroll += .005;
      scrollStream << (screen_.ActiveWindow().CurrentLine() + 1) << "/" << screen_.ActiveWindow().BufferSize() << " -- ";

      if (screen_.ActiveWindow().BufferSize() > screen_.MaxRows())
      {
         if (currentScroll <= .010)
         {
            scrollStream << "Top ";
         }
         else if (currentScroll >= 1.0)
         {
            scrollStream << "Bot ";
         }
         else
         {
            scrollStream << std::setw(2) << static_cast<int>(currentScroll * 100) << "%%";
         }
      }
   }

   return scrollStream.str();
}

std::string Normal::StateString()
{
   std::string toggles = "";

   std::string const random    = (clientState_.Random() == true) ? "random, " : "";
   std::string const repeat    = (clientState_.Repeat() == true) ? "repeat, " : "";
   std::string const single    = (clientState_.Single() == true) ? "single, " : "";
   std::string const consume   = (clientState_.Consume() == true) ? "consume, " : "";
   std::string const crossfade = (clientState_.Crossfade() > 0) ? "crossfade, " : "";

   if ((random != "") || (repeat != "") || (single != "") || (consume != "") || (crossfade != ""))
   {
      toggles += " [ON: ";
      toggles += random + repeat + single + consume + crossfade;
      toggles = toggles.substr(0, toggles.length() - 2);
      toggles += "]";
   }

   std::string volume = "";

   if (clientState_.Volume() != -1)
   {
      char vol[8];
      snprintf(vol, 8, "%d", clientState_.Volume());
      volume += " [Volume: " + std::string(vol) + "%]";
   }

   std::string updating = "";

   if (clientState_.IsUpdating() == true)
   {
      updating += " [Updating]";
   }

   std::string const currentState("[State: " + clientState_.CurrentState() + "]" + volume + toggles + updating);
   return currentState;
}
/* vim: set sw=3 ts=3: */
