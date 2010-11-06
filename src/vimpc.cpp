#include "vimpc.hpp"

#include "actions.hpp"
#include "commands.hpp"
#include "config.hpp"
#include "dbc.hpp"
#include "handler.hpp"
#include "settings.hpp"
#include "search.hpp"

using namespace Main;

Vimpc::Vimpc() :
   input_      (0),
   currentMode_(Normal),
   settings_   (Main::Settings::Instance()),
   screen_     (client_, settings_), // \todo surely this use of screen_/client_ coupling is bad
   client_     (screen_)
{
   handlerTable_[Normal]  = new Ui::Actions (screen_, client_);
   handlerTable_[Command] = new Ui::Commands(screen_, client_, settings_);
   handlerTable_[Search]  = new Ui::Search  (screen_, client_, settings_);

   ENSURE(handlerTable_.size() == ModeCount);
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
   Ui::Commands * commandHandler = dynamic_cast<Ui::Commands *>(handlerTable_[Command]);

   ASSERT(commandHandler != NULL);

   if (Config::ExecuteConfigCommands(*commandHandler) == true)
   {
      client_.Start();
      screen_.Start();

      handlerTable_[currentMode_]->InitialiseMode();

      while (Handle(Input()) == true);
   }
}


bool Vimpc::Handle(int input)
{
   Ui::Handler * handler = handlerTable_[currentMode_];

   ASSERT(handler != NULL);

   // Input must be handled before mode is changed
   input_ = input;
   bool const result = handler->Handle(input_);

   if (RequiresModeChange() == true)
   {
      ChangeMode();
   }

   return result;
}


bool Vimpc::RequiresModeChange() const
{
   return (currentMode_ != ModeAfterInput());
}

Vimpc::Mode Vimpc::ModeAfterInput() const
{
   Mode newMode = currentMode_;

   if (((currentMode_ == Command) || (currentMode_ == Search)) && ((input_ == '\n') || (input_ == 27)))
   {
      newMode = Normal;
   }
   else if ((currentMode_ == Normal) && (input_ == ':'))
   {
      newMode = Command;
   }
   else if ((currentMode_ == Normal) && (input_ == '/'))
   {
      newMode = Search;
   }

   return newMode;
}

void Vimpc::ChangeMode()
{
   Mode const oldMode = currentMode_;
   Mode const newMode = ModeAfterInput();

   if (newMode != oldMode)
   {
      currentMode_ = newMode;
      handlerTable_[oldMode]->FinaliseMode();
      handlerTable_[newMode]->InitialiseMode();
   }
}


int Vimpc::Input() const
{
   return screen_.WaitForInput();
}

