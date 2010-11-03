#include "console.hpp"

#include "screen.hpp"

#include <algorithm>

using namespace Ui;

ConsoleWindow::ConsoleWindow(Ui::Screen const & screen) :
   Window(screen)
{
}

ConsoleWindow::~ConsoleWindow()
{
}


void ConsoleWindow::Print(uint32_t line) const
{
   uint32_t currentLine(FirstLine() + line);

   if (currentLine < BufferSize())
   {
      std::string output = buffer_[currentLine].substr(0, buffer_[currentLine].find("\n"));
      mvwprintw(N_WINDOW(), line, 0, "%s", output.c_str());
   }
}


void ConsoleWindow::OutputLine(char const * const fmt, ...)
{
   static uint16_t const InputBufferSize = 256;
   char   buf[InputBufferSize];

   va_list args;
   va_start(args, fmt);
   vsnprintf(buf, InputBufferSize - 1, fmt, args);
   va_end(args);

   if ((AutoScroll() == true) && (BufferSize() == ScrollLine()))
   {
      wscrl(N_WINDOW(), 1);
      SetScrollLine(ScrollLine() + 1);
   }

   buffer_.insert(buffer_.end(), buf);
}

void ConsoleWindow::Clear() 
{ 
   buffer_.clear(); 
   werase(N_WINDOW());
   ResetScroll(); 
}
