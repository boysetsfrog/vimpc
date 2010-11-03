#include "actions.hpp"
#include "playlist.hpp"
#include "screen.hpp"
#include "vimpc.hpp"

#include <limits>

using namespace Ui;

Actions::Actions(Ui::Screen & screen, Mpc::Client & client) :
   Player           (screen, client),
   window_          (NULL),
   actionCount_     (0),
   lastAction_      (0),
   lastActionCount_ (0),
   client_          (client),
   screen_          (screen)
{
   // \todo figure out how to do alt + ctrl key combinations
   // for things like Ctrl+u and alt+1

   // \todo add proper handling of combination actions ie 'gt' and 'gg' etc
   // \todo display current count somewhere
   actionTable_['.'] = &Actions::RepeatLastAction;
   actionTable_['c'] = &Actions::ClearScreen;
   actionTable_['f'] = &Actions::ScrollToCurrent;
   actionTable_['h'] = &Actions::Previous;
   actionTable_['j'] = &Actions::ScrollDown;
   actionTable_['k'] = &Actions::ScrollUp;
   actionTable_['l'] = &Actions::Next;
   actionTable_['p'] = &Actions::Pause;
   actionTable_['r'] = &Actions::Random;
   actionTable_['s'] = &Actions::Stop;

   actionTable_[KEY_LEFT]  = actionTable_['h'];
   actionTable_[KEY_RIGHT] = actionTable_['l'];
   actionTable_[KEY_DOWN]  = actionTable_['j'];
   actionTable_[KEY_UP]    = actionTable_['k'];

   actionTable_['\n']      = &Actions::Confirm;
   actionTable_[KEY_ENTER] = &Actions::Confirm;
   actionTable_[KEY_HOME]  = &Actions::ScrollToStart;
   actionTable_[KEY_END]   = &Actions::ScrollToEnd;
   actionTable_[KEY_PPAGE] = &Actions::ScrollPageUp;
   actionTable_[KEY_NPAGE] = &Actions::ScrollPageDown;

   window_ = screen.CreateModeWindow();
}

Actions::~Actions()
{
   delete window_;
   window_ = NULL;
}

void Actions::InitialiseMode()
{
   actionCount_ = 0;

   window_->SetLine("");
}

void Actions::FinaliseMode()
{
}

bool Actions::Handle(int input)
{
   // \todo work out how to handle 
   // ALT+<number> to change windows
   bool result = true;

   if ((input >= '0') && (input <= '9'))
   {
      uint64_t const newActionCount = ((static_cast<uint64_t>(actionCount_) * 10) + (input - '0'));

      if (newActionCount <= std::numeric_limits<uint32_t>::max())
      {
         actionCount_ = newActionCount;
      }
   }
   // \todo use a symbol
   else if (input == 27)
   {
      actionCount_ = 0;
   }

   else if (actionTable_.find(input) != actionTable_.end())
   {
      uint32_t count = (actionCount_ > 0) ? actionCount_ : 1;

      if (input != '.')
      {
         lastAction_      = input;
         lastActionCount_ = actionCount_;
      }

      ptrToMember actionFunc = actionTable_[input];
      result = (*this.*actionFunc)(count);
      actionCount_ = 0;

      screen_.Update();
   }

   window_->SetLine("LAST: %u%c COUNT: %u", lastActionCount_, lastAction_, actionCount_);

   return result;
}

bool Actions::Confirm(uint32_t count)
{
   screen_.Confirm();
   return true;
}

// \todo figure out a better way
// to have the scrolling functions combined into one
bool Actions::ScrollToCurrent(uint32_t count)
{
   screen_.PlaylistWindow().ScrollTo(GetCurrentSong() + 1);
   return true;
}

bool Actions::ScrollToStart(uint32_t count)
{
   screen_.ScrollTo(0);
   return true;
}

bool Actions::ScrollToEnd(uint32_t count)
{
   screen_.ScrollTo(client_.TotalNumberOfSongs());
   return true;
}

bool Actions::ScrollPageUp(uint32_t count)
{
   return ScrollPage(Up, count);
}

bool Actions::ScrollPageDown(uint32_t count)
{
   return ScrollPage(Down, count);
}

bool Actions::ScrollPage(Direction direction, int64_t count)
{
   count *= ((screen_.MaxRows() + 1) / 2);
   count *= (direction == Ui::Actions::Up) ? -1 : 1;
   screen_.Scroll(count);
   return true;
}

bool Actions::ScrollUp(uint32_t count)
{
   return Scroll(Up, count);
}

bool Actions::ScrollDown(uint32_t count)
{
   return Scroll(Down, count);
}

bool Actions::Scroll(Direction direction, int64_t count)
{
   count *= (direction == Ui::Actions::Up) ? -1 : 1;
   screen_.Scroll(count);
   return true;
}


bool Actions::RepeatLastAction(uint32_t count)
{
   actionCount_ = (actionCount_ > 0) ? count : lastActionCount_;

   if (lastAction_ != 0)
   {
      Handle(lastAction_);
   }

   return true;
}
