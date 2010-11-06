#ifndef __UI__SEARCH
#define __UI__SEARCH

#include <string>
#include <map>
#include <vector>

#include "handler.hpp"
#include "inputmode.hpp"
#include "modewindow.hpp"
#include "player.hpp"

namespace Main
{
   class Settings;
   class Vimpc;
}

namespace Ui
{
   class Screen;

   // Handles all input received whilst in search mode
   class Search : public InputMode, public Player
   {

   public:
      Search(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings);
      ~Search();

   private:
      bool InputModeHandler(std::string input);

   private: 
      Main::Settings & settings_;
      ModeWindow     * window_;
      Ui::Screen     & screen_;

  };
}

#endif
