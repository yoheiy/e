#include <termios.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sys/ioctl.h>
#define COLOUR_ESC "\033"
#define COLOUR_RED     COLOUR_ESC "[31m"
#define COLOUR_CYAN    COLOUR_ESC "[36m"
#define COLOUR_GREY    COLOUR_ESC "[38;5;248m"
#define COLOUR_NORMAL  COLOUR_ESC "[0m"
#define COLOUR_GREY_BG COLOUR_ESC "[48;5;248m"

#include "str.h"
#include "buf.h"
#include "view.h"
#include "table_view.h"
// #include "app.h"

extern "C" {
   int tc_init();
   int tc(const char *);
}

namespace e {

void eol_out();
int  lnum_col(int n);
void lnum_padding_out(int n);
int  max(int a, int b);

extern std::vector<const char *> keywords;

void
TableView::cursor_move_word_next(int (*f)(int))
{
   const int row_prev = cursor_row_ + window_offset_;
   if (row_prev < 0 || row_prev >= buf_->num_of_lines()) return;
   Str s { buf_->get_line(row_prev) };
   if (cursor_column_ > s.len()) return;

   for (int i = cursor_column_ + 1; i < s.len(); i++)
      if (s[i] == ':') {
         cursor_column_ = i + 1;
         break; }
}

void
TableView::cursor_move_word_prev(int (*f)(int))
{
}

void
TableView::cursor_move_row_rel(int n)
{
   const int row_prev = cursor_row_ + window_offset_;
   const int col_prev = cursor_column_;

   cursor_row_ += n;
   cursor_column_ = 0;

   const int row_next = cursor_row_ + window_offset_;

   if (row_prev < 0 || row_prev >= buf_->num_of_lines()) return;
   int c = 0;
   const char *s { buf_->get_line(row_prev) };
   for (int i = 0; i <= col_prev; i++)
      if (s[i] == ':') c++;

   if (row_next < 0 || row_next >= buf_->num_of_lines()) return;
   int c2 = 0;
   const char *t { buf_->get_line(row_next) };
   for (int i = 0; i < strlen(t); i++)
      if (t[i] == ':' && ++c2 == c) {
         Str s { t };
         cursor_column_ = s.index_bytes_to_chars(i) + 1;
         break; }
}

void
tableview_keyword_hilit_colour(const char *s, int col)
{
   Str str { s };
   int pos_start = 0;
   int len = str.len();
   char *buf = (char *)malloc(str.len());
   if (!buf) {
      std::cout << s;
      return; }
   memset(buf, ' ', len);

   for (;;) {
      int pos_found = str.search_word(keywords, pos_start);
      if (pos_found == -1) break;
      auto w = str.match_word(keywords, pos_found);
      if (!w) { pos_start++; continue; }
      int len = Str(w).len();
      memset(&buf[pos_found], '~', len);
      pos_start = pos_found + len; }
   if (col >= 0 && col < len)
      buf[col] = '^';

   const int cell_width = 16;
   int cell_col = 0;
   for (int i = 0; i < len; i++) {
      if (str[i] == ':') {
         for (int i = cell_col; i < cell_width; i++)
            std::cout << ' ';
         if (i == col)
            std::cout << COLOUR_GREY_BG << ':' << COLOUR_NORMAL;
         else
            std::cout << COLOUR_CYAN << '|' << COLOUR_NORMAL;
         cell_col = 1;
         continue;
      }
      if (cell_col == cell_width - 1 && str[i + 1] != ':') {
         int found = 0;
         for (int j = i; j < len && str[j] != ':'; j++)
            if (j == col) found = 1;
         if (found) {
            std::cout << COLOUR_GREY_BG;
            str.output_char(col);
            std::cout << COLOUR_NORMAL; }
         else
            std::cout << COLOUR_CYAN << '>' << COLOUR_NORMAL;
         cell_col++; }
      if (cell_col >= cell_width) continue;

      if ((!i || buf[i - 1] != '~') && buf[i] == '~')
         std::cout << COLOUR_RED;
      if (i && buf[i - 1] == '~' && buf[i] != '~')
         std::cout << COLOUR_NORMAL;
      if (buf[i] == '^')
         std::cout << COLOUR_GREY_BG;
      str.output_char(i);
      if (buf[i] == '^')
         std::cout << COLOUR_NORMAL;
      cell_col++; }

   // EOL
   std::cout << COLOUR_NORMAL;
   if (col == len)
      std::cout << COLOUR_GREY_BG;
   else
      std::cout << COLOUR_GREY;
   std::cout << '$' << COLOUR_NORMAL;
}

void
show_content_under_cursor(const char *str, int col)
{
   Str s(str);
   int cursor_cell = 0;
   for (int i = 0; i < s.len() && i < col; i++)
      if (s[i] == ':') cursor_cell = i + 1;
   for (int j = cursor_cell; j < s.len() && s[j] != ':'; j++) {
      if (j == col) std::cout << COLOUR_GREY_BG;
      s.output_char(j);
      if (j == col) std::cout << COLOUR_NORMAL; }
   if (s[col] == ':')
      std::cout << COLOUR_GREY_BG << ':' << COLOUR_NORMAL;
   if (col == s.len())
      std::cout << COLOUR_GREY_BG << '$' << COLOUR_NORMAL;
   eol_out();
}

void
TableView::show()
{
   std::vector<const char *> v;
   const int from = window_offset_, to = window_offset_ + window_height_;
   const int cursor_line = window_offset_ + cursor_row_;
   const int lnum_col_max = max(lnum_col(from), lnum_col(to - 1));

   buf_->show(v, from, to);

   std::cout << COLOUR_GREY_BG;
   std::cout << "== " << buf_->filename() <<
                (buf_->new_file() ? " N" : buf_->dirty() ? " *" : "") <<
                " [" << from << ":" << to << "] ==";
   eol_out();
   std::cout << COLOUR_NORMAL;

   if (cursor_line >= 0 && cursor_line < buf_->num_of_lines())
      show_content_under_cursor(buf_->get_line(cursor_line), cursor_column_);
   else
      eol_out();

   std::cout << COLOUR_GREY;
   for (int i = from; i < to && i < 0; i++) {
      lnum_padding_out(lnum_col_max - lnum_col(i));
      std::cout << i << (i == cursor_line ? '>' : '#');
      eol_out(); }
   std::cout << COLOUR_NORMAL;

   int n = (from < 0) ? 0 : from;
   for (auto i : v) {
      lnum_padding_out(lnum_col_max - lnum_col(n));
      std::cout << COLOUR_GREY << n << ": " << COLOUR_NORMAL;
      tableview_keyword_hilit_colour(i, n == cursor_line ? cursor_column_ : -1);
      eol_out();
      ++n; }

   std::cout << COLOUR_GREY;
   for (int i = n; i < to; i++) {
      lnum_padding_out(lnum_col_max - lnum_col(i));
      std::cout << i << (i == cursor_line ? '>' : '#');
      eol_out(); }

   show_keywords();
   show_rot13();
   std::cout << COLOUR_NORMAL;
}

} // namespace
