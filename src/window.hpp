#ifndef __UI__WINDOW
#define __UI__WINDOW

#include <ncurses.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace Ui
{
   class Screen;

   class Window
   {
   public:
      Window(Ui::Screen const & screen);
      Window(Ui::Screen const & screen, int h, int w, int x, int y);
      virtual ~Window();

   public:
      virtual void Print(uint32_t line) const = 0;
      virtual void Confirm() const;
      virtual void Scroll(int32_t scrollCount);
      virtual void ScrollTo(uint16_t scrollLine);
      virtual void Search(std::string const & searchString) const;

   public:
      void Erase();
      void Refresh();

   public:
      void SetAutoScroll(bool autoScroll);
      bool AutoScroll() const;

   protected:
      uint16_t FirstLine() const;
      void ResetScroll(); 
      void SetScrollLine(uint16_t scrollLine);
      uint16_t ScrollLine() const;

   protected:
      WINDOW * const N_WINDOW() const { return window_;}

   private:
      virtual size_t BufferSize() const = 0;

   protected:
      Ui::Screen const & screen_;
      WINDOW   * window_;
      uint16_t    scrollLine_;
      bool       autoScroll_;
   };
}

#endif
