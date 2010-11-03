#ifndef __MAIN__VIMPC
#define __MAIN__VIMPC

#include <map>

#include "mpdclient.hpp"
#include "screen.hpp"

namespace Ui
{
   class Handler;
}

namespace Main
{
   class Settings;

   class Vimpc
   {
   public:
      Vimpc();
      ~Vimpc();

   public:
      void Run();

   private:
      typedef enum
      {
         Command,
         Normal,
         ModeCount
      } Mode;

      typedef std::map<Mode, Ui::Handler *> HandlerTable;

   private:
      bool Handle(int input);

   private:
      bool RequiresModeChange() const;
      Mode ModeAfterInput() const;
      void ChangeMode();

   private:
      int Input() const;

   private:
      int          input_;
      Mode         currentMode_;

      Settings &   settings_;
      Ui::Screen   screen_;
      Mpc::Client  client_;
      HandlerTable handlerTable_;
   };
}

#endif
