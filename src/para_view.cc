#include <iostream>
#include <vector>

#include "str.h"
#include "buf.h"
#include "view.h"
#include "para_view.h"
#include "colour.h"

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
   std::cout << COLOUR_HEADLINE;
   std::cout << "== " << buf_->filename_of_line(current_line()) <<
                (buf_->new_file() ? " N" : buf_->dirty() ? " *" : "") <<
                " [par] ==";
   eol_out();
   std::cout << COLOUR_NORMAL;
}

void
ParaView::show()
{
   mode_line();
   eol_out();

   std::vector<int> ll;
   for (int i = 0; i < buf_->num_of_lines(); i++)
      if (i == 0 || (!*buf_->get_line(i - 1) &&
                      *buf_->get_line(i)))
         ll.push_back(i);

   for (int i = 0; i < window_height_; i++) {
      int x = window_offset_ + i;
      if (x < 0 || x >= ll.size()) { eol_out(); continue; }
      int k = ll[x];
      lnum_padding_out(lnum_col(buf_->num_of_lines()) - lnum_col(k));
      std::cout << COLOUR_LINE_NR << k << ": " << COLOUR_NORMAL;
      if (i == cursor_row_) std::cout << COLOUR_CURSOR;
      std::cout << buf_->get_line(k) << " ...";
      if (i == cursor_row_) std::cout << COLOUR_NORMAL;
      eol_out();
   }
}

void
ParaView::transpose_lines()
{
   int n = 0,
       n0 = 0,
       n1 = 0,
       n2 = 0;

   for (int i = 0; i < buf_->num_of_lines(); i++) {
      if (i == 0 || (!*buf_->get_line(i - 1) &&
                      *buf_->get_line(i))) {
         if (n == window_offset_ + cursor_row_ + 0) n0 = i;
         if (n == window_offset_ + cursor_row_ + 1) n1 = i;
         if (n == window_offset_ + cursor_row_ + 2) n2 = i;
         n++; } }

   if (!n1--) return;
   if (!n2--) n2 = buf_->num_of_lines();
   buf_->rotate_lines(n1, /*range=*/n2 - n1, /*dist=*/1);
   buf_->rotate_lines(n0, /*range=*/n2 - n0, /*dist=*/n1 - n0);
}

int
ParaView::current_line()
{
   int n = 0;

   for (int i = 0; i < buf_->num_of_lines(); i++) {
      if (i == 0 || (!*buf_->get_line(i - 1) &&
                      *buf_->get_line(i))) {
         if (n == window_offset_ + cursor_row_)
            return i;
         n++; } }

   return buf_->num_of_lines();
}

void
ParaView::set_current_line(int l)
{
   int n = 0;

   for (int i = 0; i < buf_->num_of_lines(); i++) {
      if ((i == 0 || !*buf_->get_line(i - 1)) && *buf_->get_line(i))
         n++;
      if (i == l) {
         window_offset_ = n - cursor_row_ - 1;
         return; } }
}

} // namespace
