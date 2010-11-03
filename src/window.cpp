#include "window.hpp"

#include <iostream>

#include "screen.hpp"

using namespace Ui;

Window::Window(Ui::Screen const & screen) :
   screen_    (screen),
   window_    (NULL),
   scrollLine_(screen_.MaxRows()),
   autoScroll_(false)
{
   window_ = newwin(screen_.MaxRows(), screen_.MaxColumns(), 0, 0);
}

Window::Window(Ui::Screen const & screen, int h, int w, int x, int y) :
   screen_    (screen),
   window_    (NULL),
   scrollLine_(h),
   autoScroll_(false)
{
   window_ = newwin(h, w, x, y);
}

Window::~Window()
{
   delwin(window_);
}

void Window::Confirm() const
{
}

void Window::Scroll(int32_t scrollCount)
{
   uint16_t const newLine = (scrollLine_ + scrollCount);

   if (newLine < screen_.MaxRows())
   {
      scrollLine_ = screen_.MaxRows();
   }
   else if (newLine > BufferSize())
   {
      scrollLine_ = BufferSize();
   }
   else
   {
      scrollLine_ = newLine;
   }
}

void Window::ScrollTo(uint16_t scrollLine)
{
   scrollLine_ = scrollLine + (screen_.MaxRows() / 2);

   if (scrollLine_ < screen_.MaxRows())
   {
      scrollLine_ = screen_.MaxRows();
   }
   else if (scrollLine_ > BufferSize())
   {
      scrollLine_ = BufferSize();
   }
}


void Window::Erase()
{
   werase(N_WINDOW());
}

void Window::Refresh()
{
   wnoutrefresh(N_WINDOW());
}


void Window::SetAutoScroll(bool autoScroll)
{
   autoScroll_ = autoScroll;
}

bool Window::AutoScroll() const
{
   return autoScroll_;
}


uint16_t Window::FirstLine() const
{
   uint16_t result = 0;

   if ((scrollLine_ - screen_.MaxRows()) > 0)
   {
      result = (scrollLine_ - screen_.MaxRows());
   }

   return result;
}

void Window::ResetScroll()
{
   scrollLine_ = screen_.MaxRows();
}

uint16_t Window::ScrollLine() const
{
   return scrollLine_;
}

void Window::SetScrollLine(uint16_t scrollLine)
{
   scrollLine_ = scrollLine;
}
