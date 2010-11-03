#ifndef __UI__CONSOLE
#define __UI__CONSOLE

#include "window.hpp"

#include <string>

namespace Ui
{
   class ConsoleWindow : public Ui::Window
   {
   public:
      ConsoleWindow(Ui::Screen const & screen);
      ~ConsoleWindow();

   public:
      void Print(uint32_t line) const;

   public:
      void OutputLine(char const * const fmt, ...);
      void Clear();

   private:
      size_t BufferSize() const { return buffer_.size(); }

   private:
      typedef std::vector<std::string> WindowBuffer;
      WindowBuffer buffer_;
   };
}

#endif
