#include <iostream>
#define COLOUR_ESC "\033"
#define COLOUR_RED     COLOUR_ESC "[31m"
#define COLOUR_CYAN    COLOUR_ESC "[36m"
#define COLOUR_GREY    COLOUR_ESC "[38;5;248m"
#define COLOUR_NORMAL  COLOUR_ESC "[0m"
#define COLOUR_GREY_BG COLOUR_ESC "[48;5;248m"

#include "str.h"
#include "buf.h"
#include "view.h"
#include "para_view.h"

extern "C" {
   int tc_init();
   int tc(const char *);
}

namespace e {

void eol_out();
int  lnum_col(int n);
void lnum_padding_out(int n);

void
ParaView::mode_line()
{
   std::cout << COLOUR_GREY_BG;
   std::cout << "== " << buf_->filename() <<
                (buf_->new_file() ? " N" : buf_->dirty() ? " *" : "") <<
                " [par] ==";
   eol_out();
   std::cout << COLOUR_NORMAL;
}

void
ParaView::show()
{
   mode_line();

   int n = 0;
   for (int i = window_offset_; i < buf_->num_of_lines(); i++) {
      if (i == 0 || (!*buf_->get_line(i - 1) &&
                      *buf_->get_line(i))) {
         lnum_padding_out(lnum_col(buf_->num_of_lines()) - lnum_col(i));
         std::cout << COLOUR_GREY << i << ": " << COLOUR_NORMAL;
         if (n == cursor_row_) std::cout << COLOUR_GREY_BG;
         std::cout << buf_->get_line(i) << " ...";
         if (n == cursor_row_) std::cout << COLOUR_NORMAL;
         eol_out();
         n++; }

      if (n >= window_height_) break; }
}

void
ParaView::transpose_lines()
{
   int n = 0,
       n0 = 0,
       n1 = 0,
       n2 = 0;

   for (int i = window_offset_; i < buf_->num_of_lines(); i++) {
      if (i == 0 || (!*buf_->get_line(i - 1) &&
                      *buf_->get_line(i))) {
         if (n == cursor_row_ + 0) n0 = i;
         if (n == cursor_row_ + 1) n1 = i;
         if (n == cursor_row_ + 2) n2 = i;
         n++; } }

   if (!n1) return;
   if (!n2) goto special;
   buf_->rotate_lines(n0, /*range=*/n2 - n0, /*dist=*/n1 - n0);
   return;

special:
   n2 = buf_->num_of_lines();

   buf_->insert_empty_line(n2);
   buf_->rotate_lines(n0, /*range=*/n2 + 1 - n0, /*dist=*/n1 - n0);
   buf_->delete_line(n2);
}

} // namespace
