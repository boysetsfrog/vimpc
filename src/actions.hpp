#ifndef __UI__ACTIONS
#define __UI__ACTIONS

#include <map>

#include "handler.hpp"
#include "modewindow.hpp"
#include "player.hpp"

namespace Main
{
   class Vimpc;
}

namespace Ui
{
   // Handles all input received whilst in normal mode
   class Actions : public Handler, public Player
   {
   public:
      Actions(Ui::Screen & screen, Mpc::Client & client);
      ~Actions();

   public: // Ui::Handler Inherits
      void InitialiseMode();
      void FinaliseMode();
      bool Handle(int input);

   private:
      typedef enum
      {
         Up,
         Down
      } Direction;

   private:
      bool Confirm(uint32_t count);
      bool ScrollToCurrent(uint32_t count);
      bool ScrollToStart(uint32_t count);
      bool ScrollToEnd(uint32_t count);
      bool ScrollPageUp(uint32_t);
      bool ScrollPageDown(uint32_t);
      bool ScrollPage(Direction, int64_t);
      bool ScrollUp(uint32_t);
      bool ScrollDown(uint32_t);
      bool Scroll(Direction, int64_t);

      bool RepeatLastAction(uint32_t);

   private:
      ModeWindow * window_;

      uint32_t actionCount_;
      int32_t  lastAction_;
      uint32_t lastActionCount_;

      Mpc::Client & client_;
      Ui::Screen  & screen_;

      typedef bool (Ui::Actions::*ptrToMember)(uint32_t);
      typedef std::map<int, ptrToMember> ActionTable;
      ActionTable actionTable_;
   };
}

#endif
