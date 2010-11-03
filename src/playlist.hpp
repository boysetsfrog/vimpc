#ifndef __UI__PLAYLIST
#define __UI__PLAYLIST

#include "song.hpp"
#include "window.hpp"

namespace Mpc
{
   class Client;
}

namespace Ui
{
   class PlaylistWindow : public Ui::Window
   {
   public:
      PlaylistWindow(Ui::Screen const & screen, Mpc::Client& client);
      ~PlaylistWindow();

   public:
      void AddSong(Mpc::Song * newSong);

   public:
      void Print(uint32_t line) const;
      void Confirm() const;
      void Scroll(int32_t scrollCount);
      void ScrollTo(uint16_t scrollLine);

   private:
      size_t BufferSize() const { return buffer_.size(); }

   private:
      int64_t LimitCurrentSelection(int64_t currentSelection) const;

   private:
      int64_t currentSelection_;

      Mpc::Client & client_;

      typedef std::vector<Mpc::Song *> SongBuffer;
      SongBuffer buffer_;
   };
}

#endif
