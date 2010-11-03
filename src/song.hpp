#ifndef __MPC_SONG
#define __MPC_SONG

#include <stdint.h>
#include <string>

namespace Mpc
{
   class Song
   {
   public:
      Song(uint32_t id) :
         id_(id)
      { }

      ~Song()
      { }

   public:
      int32_t Id()
      {
         return id_;
      }

      void SetArtist(const char * artist)
      {
         if (artist != NULL)
         {
            artist_ = artist;
         }
         else
         {
            artist_ = "Unknown";
         }
      }

      std::string & Artist()
      {
         return artist_;
      }

      void SetTitle(const char * title)
      {
         if (title != NULL)
         {
            title_ = title;
         }
         else
         {
            title_ = "Unknown";
         }
      }

      std::string & Title()
      {
         return title_;
      }

   private:
      int32_t     id_;
      std::string artist_;
      std::string title_;
   };
}

#endif
