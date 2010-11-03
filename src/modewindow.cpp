#include "modewindow.hpp"

#include <ncurses.h>

#include "screen.hpp"

using namespace Ui;

ModeWindow::ModeWindow(Ui::Screen const & screen) :
   Window         (screen, 0, screen.MaxColumns(), screen.MaxRows() + 1, 0),
   cursorVisible_ (false),
   cursorPosition_(0)
{
}

ModeWindow::~ModeWindow()
{
}

void ModeWindow::SetLine(char const * const fmt, ...)
{
   static uint16_t const InputBufferSize = 256;
   char   buf[InputBufferSize];

   va_list args;
   va_start(args, fmt);
   vsnprintf(buf, InputBufferSize - 1, fmt, args);
   va_end(args);

   buffer_ = buf;
   Print(1);
}

void ModeWindow::Scroll(int32_t scrollCount)
{
}

void ModeWindow::Print(uint32_t line) const
{
   WINDOW * window = N_WINDOW();

   curs_set((cursorVisible_ == true) ? 2 : 0);

   werase(window);
   mvwprintw(window, 0, 0, buffer_.c_str());
   wmove(window, 0, cursorPosition_);
   wrefresh(window);
}

size_t ModeWindow::BufferSize() const
{
   return 1;
}

void ModeWindow::SetCursorPosition(uint32_t cursorPosition)
{
   cursorPosition_ = cursorPosition;
}

void ModeWindow::ShowCursor()
{
   cursorVisible_ = true;
}

void ModeWindow::HideCursor()
{
   cursorVisible_ = false;
}
