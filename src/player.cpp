#include "player.hpp"

#include "console.hpp"
#include "mpdclient.hpp"
#include "screen.hpp"

#include <stdlib.h>

using namespace Ui;

Player::Player(Ui::Screen & screen, Mpc::Client & client) :
   screen_(screen),
   client_(client)
{

}

Player::~Player()
{

}

bool Player::Next(uint32_t count)
{
   if (count <= 1)
   {
      Next("");
   }
   else
   {
      client_.Play(GetCurrentSong() + count);
   }

   return true;
}

bool Player::Previous(uint32_t count)
{
   if (count <= 1)
   {
      Previous("");
   }
   else
   {
      client_.Play(GetCurrentSong() - count);
   }

   return true;
}

bool Player::Connect(std::string const & arguments)
{
   client_.Connect();
   return true;
}

bool Player::ClearScreen(std::string const & arguments)
{
   screen_.Clear();
   return true;
}

bool Player::Quit(std::string const & arguments)
{
   return false;
}

bool Player::Echo(std::string const & arguments)
{
   screen_.ConsoleWindow().OutputLine("%s", arguments.c_str());
   return true;
}

bool Player::Play(std::string const & arguments)
{
   client_.Play(atoi(arguments.c_str()) - 1);
   return true;
}

bool Player::Pause(std::string const & arguments)
{
   client_.Pause();
   return true;
}

bool Player::Next(std::string const & arguments)
{
   client_.Next();
   return true;
}

bool Player::Previous(std::string const & arguments)
{
   client_.Previous();
   return true;
}

bool Player::Stop(std::string const & arguments)
{
   client_.Stop();
   return true;
}

bool Player::Random(std::string const & arguments)
{
   const bool randomOn = (arguments.compare("on") == 0);

   client_.Random(randomOn);
   return true;
}


bool Player::Console(std::string const & arguments)
{
   screen_.SetActiveWindow(Ui::Screen::Console);
   return true;
}

bool Player::Help(std::string const & arguments)
{
   screen_.SetActiveWindow(Ui::Screen::Help);
   return true;
}

bool Player::Playlist(std::string const & arguments)
{
   screen_.SetActiveWindow(Ui::Screen::Playlist);
   return true;
}

bool Player::Library(std::string const & arguments)
{
   screen_.SetActiveWindow(Ui::Screen::Library);
   return true;
}


uint32_t Player::GetCurrentSong() const
{
   return client_.GetCurrentSong();
}
