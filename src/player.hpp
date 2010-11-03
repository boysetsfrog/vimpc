#ifndef __UI__PLAYER
#define __UI__PLAYER

#include <string>

#include <stdint.h>

namespace Mpc
{
   class Client;
}

namespace Ui
{
   class Screen;
}

namespace Ui
{
   // Class for functionality that is shared between all
   // modes and needs to be accessed via actions or commands, etc
   class Player
   {
   public:
      Player(Ui::Screen & screen, Mpc::Client & client);
      virtual ~Player();

   protected:
      //Single argument commands
      bool ClearScreen(uint32_t count) { return ClearScreen(""); }
      bool Quit(uint32_t count)        { return Quit(""); }
      bool Pause(uint32_t count)       { return Pause(""); }
      bool Next(uint32_t count);
      bool Previous(uint32_t count);
      bool Stop(uint32_t count)        { return Stop(""); }
      bool Random(uint32_t count)      { return Random(""); }

      //Commands which can have multiple arguments
      bool Connect(std::string const & arguments);
      bool ClearScreen(std::string const & arguments);
      bool Quit(std::string const & arguments);
      bool Echo(std::string const & arguments);
      bool Play(std::string const & arguments);
      bool Pause(std::string const & arguments);
      bool Next(std::string const & arguments);
      bool Previous(std::string const & arguments);
      bool Stop(std::string const & arguments);
      bool Random(std::string const & arguments);

      //Commands related to windows
      bool Console(std::string const & arguments);
      bool Help(std::string const & arguments);
      bool Playlist(std::string const & arguments);
      bool Library(std::string const & arguments);

   protected:
      uint32_t GetCurrentSong() const;

   protected:
      Ui::Screen  & screen_;
      Mpc::Client & client_;
   };
}

#endif
