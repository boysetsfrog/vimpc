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

   vimpc.hpp - handles mode changes and input processing
   */

#include "vimpc.hpp"

// Mode handlers
#include "handler.hpp"
#include "normal.hpp"
#include "command.hpp"
#include "search.hpp"

#include "assert.hpp"
#include "config.hpp"
#include "settings.hpp"

using namespace Main;

Vimpc::Vimpc() :
   currentMode_ (Normal),
   settings_    (Main::Settings::Instance()),
   screen_      (client_, settings_), // \todo surely this use of screen_/client_ coupling is bad
   client_      (screen_),
   handlerTable_()
{
   Ui::Search * const search   = new Ui::Search (screen_, client_, settings_);
   handlerTable_[Command]      = new Ui::Command(screen_, client_, settings_);
   handlerTable_[Normal]       = new Ui::Normal (screen_, client_, settings_, *search);
   handlerTable_[Search]       = search;

   ENSURE(handlerTable_.size()     == ModeCount);
   ENSURE(HandlersAreInitialised() == true);
}

Vimpc::~Vimpc()
{
   for (HandlerTable::iterator it = handlerTable_.begin(); (it != handlerTable_.end()); ++it)
   {
      delete (it->second);
      it->second = NULL;
   }
}

void Vimpc::Run()
{
   Ui::Command & commandHandler = assert_reference(dynamic_cast<Ui::Command *>(handlerTable_[Command]));

   if (Config::ExecuteConfigCommands(commandHandler) == true)
   {
      Ui::Handler & handler = assert_reference(handlerTable_[currentMode_]);

      if (client_.Connected() == false)
      {
         client_.Connect("127.0.0.1");
      }

      screen_.Start();
      handler.InitialiseMode(0);

      while (Handle(Input()) == true);
   }
}


int Vimpc::Input() const
{
   return screen_.WaitForInput();
}

bool Vimpc::Handle(int input)
{
   Ui::Handler & handler = assert_reference(handlerTable_[currentMode_]);

   // Input must be handled before mode is changed
   bool const result = handler.Handle(input);

   if (RequiresModeChange(input) == true)
   {
      ChangeMode(input);
   }

   return result;
}

bool Vimpc::HandlersAreInitialised()
{
   bool result = true;

   for (HandlerTable::iterator it = handlerTable_.begin(); ((it != handlerTable_.end()) && (result == true)); ++it)
   {
      result = (it->second != NULL);
   }
   
   return result;
}


bool Vimpc::RequiresModeChange(int input) const
{
   return (currentMode_ != ModeAfterInput(input));
}

Vimpc::Mode Vimpc::ModeAfterInput(int input) const
{
   Mode newMode = currentMode_;

   // Check if we are returning to normal mode
   if (currentMode_ != Normal)
   {
      Ui::Handler const & normalHandler = assert_reference(handlerTable_.at(Normal));

      if (normalHandler.CausesModeToStart(input) == true)
      {   
         newMode = Normal;
      }
   }
   // Must be in normal mode to be able to enter any other mode
   else
   {
      for (HandlerTable::const_iterator it = handlerTable_.begin(); (it != handlerTable_.end()); ++it)
      {
         Ui::Handler const & handler = assert_reference(it->second);

         if (handler.CausesModeToStart(input) == true)
         {
            newMode = it->first;
         }
      }
   }
  
   return newMode;
}

void Vimpc::ChangeMode(int input)
{
   Mode const oldMode = currentMode_;
   Mode const newMode = ModeAfterInput(input);

   if (newMode != oldMode)
   {
      Ui::Handler & oldHandler = assert_reference(handlerTable_[oldMode]);
      Ui::Handler & newHandler = assert_reference(handlerTable_[newMode]);

      currentMode_ = newMode;

      oldHandler.FinaliseMode(input);
      newHandler.InitialiseMode(input);
   }
}
