#ifndef __UI__MODEWINDOW
#define __UI__MODEWINDOW

#include "window.hpp"

#include <stdint.h>
#include <string>

namespace Ui
{
   class ModeWindow : public Ui::Window
   {
   public:
      ModeWindow(Ui::Screen const &);
      ~ModeWindow();

   public:
      void SetLine(char const * const fmt, ... );
      void Scroll(int32_t scrollCount);
      void Print(uint32_t line) const;

      size_t BufferSize() const;
      void SetCursorPosition(uint32_t cursorPosition);
      void ShowCursor();
      void HideCursor();

   private:
      bool        cursorVisible_;
      uint32_t    cursorPosition_;
      std::string buffer_;
   };
}

#endif
