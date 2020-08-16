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
#include "isearch_view.h"

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

void
show_pat_line(const char *pat)
{
   std::cout << '[' << pat;
   std::cout << COLOUR_GREY_BG;
   std::cout << ']';
   std::cout << COLOUR_NORMAL;
   eol_out();
}

ISearchView::ISearchView(Buf *b) : FilterView(b)
{
   *pat_ = '\0';
}

void
ISearchView::char_insert(char c)
{
   const int cl = current_line();

   const int l = strlen(pat_);
   if (l + 1 == sizeof pat_) return;
   pat_[l] = c;
   pat_[l + 1] = '\0';

   int n = 0;
   for (int i = 0; i < buf_->num_of_lines(); i++) {
      if (i == cl) {
         window_offset_ = n - cursor_row_;
         return; }
      if (strstr(buf_->get_line(i), pat_))
         n++; }
   window_offset_ = n - cursor_row_ - 1;
}

void
ISearchView::char_delete_backward()
{
   const int cl = current_line();

   chop(pat_);

   int n = 0;
   for (int i = 0; i < buf_->num_of_lines(); i++) {
      if (i == cl) {
         window_offset_ = n - cursor_row_;
         return; }
      if (strstr(buf_->get_line(i), pat_))
         n++; }
}

void
ISearchView::char_delete_to_bol()
{
   const int cl = current_line();

   *pat_ = '\0';

   set_current_line(cl);
}

void
ISearchView::mode_line()
{
   std::cout << COLOUR_GREY_BG;
   std::cout << "== " << buf_->filename_of_line(current_line()) <<
                (buf_->new_file() ? " N" : buf_->dirty() ? " *" : "") <<
                " [isearch] ==";
   eol_out();
   std::cout << COLOUR_NORMAL;
}

void
ISearchView::show()
{
   mode_line();
   show_pat_line(pat_);

   std::vector<int> ll;
   for (int i = 0; i < buf_->num_of_lines(); i++)
      if (strstr(buf_->get_line(i), pat_))
         ll.push_back(i);

   for (int i = 0; i < window_height_; i++) {
      int x = window_offset_ + i;
      if (x < 0 || x >= ll.size()) {
         std::cout << (i == cursor_row_ ? COLOUR_GREY_BG : COLOUR_GREY);
         std::cout << '$' << COLOUR_NORMAL;
         eol_out();
         continue; }
      int k = ll[x];

      lnum_padding_out(lnum_col(buf_->num_of_lines()) - lnum_col(k));
      std::cout << COLOUR_GREY << k << ": " << COLOUR_NORMAL;
      if (i == cursor_row_) std::cout << COLOUR_GREY_BG;
      std::cout << buf_->get_line(k) << '$';
      if (i == cursor_row_) std::cout << COLOUR_NORMAL;
      eol_out(); }
}

int
ISearchView::current_line()
{
   int n = 0;

   for (int i = 0; i < buf_->num_of_lines(); i++) {
      const char *s = buf_->get_line(i);
      if (strstr(s, pat_)) {
         if (n == window_offset_ + cursor_row_)
            return i;
         n++; } }

   return buf_->num_of_lines();
}

void
ISearchView::set_current_line(int l)
{
   // assume pat_ == ""
   window_offset_ = l - cursor_row_;
}

} // namespace
