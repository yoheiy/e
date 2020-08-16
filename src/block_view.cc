#include <iostream>
#include <vector>
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
BlockView::cursor_move_char_abs(int n)
{
   const int cl = current_line();

   cursor_column_ = n;

   int m = 0;
   for (int i = 0; i < buf_->num_of_lines(); i++) {
      if (i == cl) {
         window_offset_ = m - cursor_row_;
         return; }
      if (count_whitespace(buf_->get_line(i)) <= cursor_column_)
         m++; }
}

void
BlockView::cursor_move_char_rel(int n)
{
   cursor_move_char_abs(cursor_column_ + n);
}

void
BlockView::show()
{
   mode_line();
   show_ruler(lnum_col(buf_->num_of_lines()) + 2, cursor_column_,
                                                        window_width_);

   std::vector<int> ll;

   for (int i = 0; i < buf_->num_of_lines(); i++)
      if (count_whitespace(buf_->get_line(i)) <= cursor_column_)
         ll.push_back(i);

   for (int i = 0; i < window_height_; i++) {
      const int x = window_offset_ + i;
      if (x < 0 || x >= ll.size()) {
         std::cout << (i == cursor_row_ ? COLOUR_GREY_BG : COLOUR_GREY);
         std::cout << '$' << COLOUR_NORMAL;
         eol_out();
         continue; }
      const int k = ll[x];

      lnum_padding_out(lnum_col(buf_->num_of_lines()) - lnum_col(k));
      std::cout << COLOUR_GREY << k << ": " << COLOUR_NORMAL;
      if (i != cursor_row_) {
         std::cout << buf_->get_line(k);
         std::cout << COLOUR_GREY << '$' << COLOUR_NORMAL; }
      else {
         Str str { buf_->get_line(k) };
         for (int j = 0; j < str.len(); j++) {
            if (j == cursor_column_) std::cout << COLOUR_GREY_BG;
            str.output_char(j);
            if (j == cursor_column_) std::cout << COLOUR_NORMAL; }
         std::cout << (cursor_column_ >= str.len() ? COLOUR_GREY_BG :
                                                     COLOUR_GREY);
         std::cout << '$' << COLOUR_NORMAL; }
      eol_out(); }
}

int
BlockView::current_line()
{
   int n = 0;

   for (int i = 0; i < buf_->num_of_lines(); i++) {
      const char *s = buf_->get_line(i);
      if (count_whitespace(s) <= cursor_column_) {
         if (n == window_offset_ + cursor_row_)
            return i;
         n++; } }

   return buf_->num_of_lines();
}

void
BlockView::set_current_line(int l)
{
   int n = 0;

   for (int i = 0; i < buf_->num_of_lines(); i++) {
      if (i == l) {
         window_offset_ = n - cursor_row_;
         return; }
      if (count_whitespace(buf_->get_line(i)) <= cursor_column_)
         n++; }
}

} // namespace
