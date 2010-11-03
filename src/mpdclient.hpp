#ifndef __MPC__CLIENT
#define __MPC__CLIENT

#include <mpd/client.h>

namespace Ui
{
   class Screen;
}

namespace Mpc
{
   class Client
   {
   public:
      Client(Ui::Screen & screen);
      ~Client();

   public:
      void Start();

      void Connect();
      void Play(uint32_t playId);
      void Pause();
      void Next();
      void Previous();
      void Stop();
      void Random(bool randomOn);

   public:
      int32_t GetCurrentSong() const;
      int32_t TotalNumberOfSongs() const;

   public:
      void DisplaySongInformation();

   private:
      void CheckError();

   private:
      bool started_;

      Ui::Screen & screen_;

      struct mpd_connection * connection_;

   };
}

#endif
