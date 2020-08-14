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
#include "block_view.h"

namespace e {

void eol_out();
int  lnum_col(int n);
void lnum_padding_out(int n);
void show_ruler(int padding, int col, int width);

void
BlockView::mode_line()
{
   std::cout << COLOUR_GREY_BG;
   std::cout << "== " << buf_->filename_of_line(current_line()) <<
                (buf_->new_file() ? " N" : buf_->dirty() ? " *" : "") <<
                " [block] ==";
   eol_out();
   std::cout << COLOUR_NORMAL;
}

int
count_whitespace(const char *s)
{
   for (int i = 0; s[i]; i++)
      if (s[i] != ' ')
         return i;

   return 0;
}

void
BlockView::show()
{
   mode_line();
   show_ruler(lnum_col(buf_->num_of_lines()) + 2, cursor_column_, window_width_);

   int n = 0;
   for (int i = window_offset_; i < buf_->num_of_lines(); i++) {
      const char *s = buf_->get_line(i);
      if (count_whitespace(s) <= cursor_column_) {
         lnum_padding_out(lnum_col(buf_->num_of_lines()) - lnum_col(i));
         std::cout << COLOUR_GREY << i << ": " << COLOUR_NORMAL;
         if (n != cursor_row_)
            std::cout << s;
         else {
            Str str { s };
            for (int j = 0; j < str.len(); j++) {
               if (j == cursor_column_) std::cout << COLOUR_GREY_BG;
               str.output_char(j);
               if (j == cursor_column_) std::cout << COLOUR_NORMAL; } }
         eol_out();
         n++; }

      if (n >= window_height_) break; }
}

int
BlockView::current_line()
{
   int n = 0;

   for (int i = window_offset_; i < buf_->num_of_lines(); i++) {
      const char *s = buf_->get_line(i);
      if (count_whitespace(s) <= cursor_column_) {
         if (n == cursor_row_)
            return i;
         n++; } }

   return buf_->num_of_lines();
}

void
BlockView::set_current_line(int l)
{
   window_offset_ = l;
   cursor_row_    = 0;
}

} // namespace
