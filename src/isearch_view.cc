#include <iostream>
#define COLOUR_ESC "\033"
#define COLOUR_RED     COLOUR_ESC "[31m"
#define COLOUR_CYAN    COLOUR_ESC "[36m"
#define COLOUR_GREY    COLOUR_ESC "[38;5;248m"
#define COLOUR_NORMAL  COLOUR_ESC "[0m"
#define COLOUR_GREY_BG COLOUR_ESC "[48;5;248m"

#include "str.h"
#include "buf.h"
#include "isearch_view.h"

extern "C" {
   int tc_init();
   int tc(const char *);
}

namespace {

char
chop(char *s)
{
   char c;
   int  l;

   l = strlen(s);
   if (!l) return '\0';
   l--;
   c = s[l];
   s[l] = '\0';
   return c;
}

}
namespace e {

void eol_out();
int  lnum_col(int n);
void lnum_padding_out(int n);

ISearchView::ISearchView(Buf *b) : FilterView(b)
{
   *pat_ = '\0';
}

void
ISearchView::char_insert(char c)
{
   const int l = strlen(pat_);
   if (l + 1 == sizeof pat_) return;
   pat_[l] = c;
   pat_[l + 1] = '\0';
}

void
ISearchView::char_delete_backward()
{
   chop(pat_);
}

void
ISearchView::char_delete_to_bol()
{
   *pat_ = '\0';
}

void
ISearchView::mode_line()
{
   std::cout << COLOUR_GREY_BG;
   std::cout << "== " << buf_->filename_of_line(current_line()) <<
                (buf_->new_file() ? " N" : buf_->dirty() ? " *" : "") <<
                " [isearch] == ";
   std::cout << COLOUR_NORMAL;
   std::cout << '[' << pat_ << ']';
   std::cout << COLOUR_GREY_BG;
   eol_out();
   std::cout << COLOUR_NORMAL;
}

void
ISearchView::show()
{
   mode_line();

   int n = 0;
   for (int i = window_offset_; i < buf_->num_of_lines(); i++) {
      const char *s = buf_->get_line(i);
      if (strstr(s, pat_)) {
         lnum_padding_out(lnum_col(buf_->num_of_lines()) - lnum_col(i));
         std::cout << COLOUR_GREY << i << ": " << COLOUR_NORMAL;
         if (n == cursor_row_) std::cout << COLOUR_GREY_BG;
         std::cout << s << '$';
         if (n == cursor_row_) std::cout << COLOUR_NORMAL;
         eol_out();
         n++; }

      if (n >= window_height_) break; }
}

int
ISearchView::current_line()
{
   int n = 0;

   for (int i = window_offset_; i < buf_->num_of_lines(); i++) {
      const char *s = buf_->get_line(i);
      if (strstr(s, pat_)) {
         if (n == cursor_row_)
            return i;
         n++; } }

   return buf_->num_of_lines();
}

void
ISearchView::set_current_line(int l)
{
   window_offset_ = l;
   cursor_row_    = 0;
}

} // namespace
