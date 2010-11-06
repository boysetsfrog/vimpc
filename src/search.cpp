#include "search.hpp"

#include <iostream>

#include "vimpc.hpp"
#include "settings.hpp"

using namespace Ui;

char const SearchPrompt   = '/';

Search::Search(Ui::Screen & screen, Mpc::Client & client, Main::Settings & settings) :
   InputMode   (SearchPrompt, screen),
   Player      (screen, client),
   settings_   (settings),
   screen_     (screen)
{
}

Search::~Search()
{
}

bool Search::InputModeHandler(std::string input)
{
   screen_.Search(input);

   return true;
}
