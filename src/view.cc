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

#include "str.h"
#include "buf.h"
#include "view.h"
#include "colour.h"

extern "C" {
   int tc_init();
   int tc(const char *);
}

namespace e {

int min(int a, int b) { return (a < b) ? a : b; }
int max(int a, int b) { return (a > b) ? a : b; }

std::vector<const char *> keywords;

View::View(Buf * buf) :
   buf_(buf),
   window_offset_(0),
   cursor_row_(0),
   cursor_column_(0)
{
   struct winsize w;

   /* see tty_ioctl(4) */
   ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
   window_width_  = w.ws_col;
   window_height_ = w.ws_row - 6;
}

void
View::window_centre_cursor() {
   struct winsize w;

   /* see tty_ioctl(4) */
   ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
   window_width_  = w.ws_col;
   window_height_ = w.ws_row - 6;

   int t = window_height_ / 2;
   window_offset_ += cursor_row_ - t;
   cursor_row_ = t;
}

int
log10(int n)
{
   int c = 0;

   while (n >= 10) {
      n /= 10;
      c++; }
   return c;
}

int lnum_col(int n) { return n < 0 ? log10(-n) + 2 : log10(n) + 1; }

void
repeated_char_out(int n, char c)
{
   for (int i = 0; i < n; i++)
      std::cout << c;
}

void
lnum_padding_out(int n)
{
   repeated_char_out(n, ' ');
}

void
eol_out()
{
   tc("ce");
   std::cout << std::endl;
}

void
keyword_hilit_uline(int n)
{
   repeated_char_out(n, '~');
}

void
show_ruler(int padding, int col, int width)
{
   std::cout << COLOUR_RULER;
   tc("ce");
   lnum_padding_out(padding);

   const int n = (width - padding - 1) / 8;
   for (int i = 0; i < n; i++)
      printf("0    %3o", i + 1);
   std::cout << '0';
   std::cout << COLOUR_NORMAL;

   tc("cr");
   for (int i = 0; i < padding + col; i++)
      tc("nd");
   std::cout << COLOUR_RULER_INDEX << '*' << COLOUR_NORMAL << std::endl;
}

void
View::keyword_hilit_colour(const char *s, int col, int width)
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

   for (int i = 0; i < len; i++) {
      if (i == width - 1) {
         std::cout << COLOUR_EOL << '>' << COLOUR_NORMAL;
         return; }
      if ((!i || buf[i - 1] != '~') && buf[i] == '~')
         std::cout << COLOUR_KEYWORD;
      if (i && buf[i - 1] == '~' && buf[i] != '~')
         std::cout << COLOUR_NORMAL;
      if (buf[i] == '^')
         std::cout << COLOUR_CURSOR;
      str.output_char(i);
      if (buf[i] == '^')
         std::cout << COLOUR_NORMAL; }

   // EOL
   std::cout << COLOUR_NORMAL;
   if (col == len)
      std::cout << COLOUR_CURSOR;
   else
      std::cout << COLOUR_EOL;
   std::cout << '$' << COLOUR_NORMAL;
   if (len + 1 == width) {
      std::cout << std::endl;
      return; }
   eol_out();
}

void
View::show_keywords()
{
   const char k[] = "[keywords";
   int n = strlen(k);
   std::cout << k;
   for (auto i : keywords) {
      int d = strlen(i) + 1;
      if (n + d > 80) {
         n = 0;
         eol_out(); }
      n += d;
      std::cout << ' ' << i; }
   std::cout << ']';
   eol_out();
}

void
View::show_rot13()
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >= buf_->num_of_lines()) return;

   const char * row_header = "r13 ";
   std::cout << row_header;

   const int col = window_width_ - strlen(row_header);

   Str s { buf_->get_line(line) };
   for (int i = 0; i < min(col, s.len()); i++) {
      int c = s[i];
      if (c >= 0x100) {
         s.output_char(i);
         continue; }

      if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M')) c += 13; else
      if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z')) c -= 13;
      std::cout << (char)c; }

   if (col > s.len()) eol_out(); else std::cout << std::endl;
}

void
View::show()
{
   std::vector<const char *> v;
   const int from = window_offset_, to = window_offset_ + window_height_;
   const int cursor_line = window_offset_ + cursor_row_;
   const int lnum_col_max = max(lnum_col(from), lnum_col(to - 1));

   buf_->show(v, from, to);

   std::cout << COLOUR_HEADLINE;
   std::cout << "== " << buf_->filename_of_line(cursor_line) <<
                (buf_->new_file() ? " N" : buf_->dirty() ? " *" : "") <<
                " [" << from << ":" << to << "] ==";
   std::cout << " { u" << buf_->undo_pos() << " }";
   eol_out();
   std::cout << COLOUR_NORMAL;

   show_ruler(lnum_col_max + 2, cursor_column_, window_width_);

   std::cout << COLOUR_LINE_NR;
   for (int i = from; i < to && i < 0; i++) {
      lnum_padding_out(lnum_col_max - lnum_col(i));
      std::cout << i << (i == cursor_line ? '>' : '#');
      eol_out(); }
   std::cout << COLOUR_NORMAL;

   int n = (from < 0) ? 0 : from;
   for (auto i : v) {
      lnum_padding_out(lnum_col_max - lnum_col(n));
      std::cout << COLOUR_LINE_NR << n << ": " << COLOUR_NORMAL;
      keyword_hilit_colour(i, n == cursor_line ? cursor_column_ : -1, window_width_ - lnum_col_max - 2);

      if (n == cursor_line) {
         int m = min(cursor_column_, buf_->line_length(n));
         int c = Str(i)[m];
         lnum_padding_out(lnum_col_max + 2 + m);
         std::cout << COLOUR_CURSOR;
         std::cout << '^';
         std::cout << m;
         std::cout << '#' << std::hex << c << std::dec;
         std::cout << COLOUR_NORMAL;
         eol_out(); }
      ++n; }

   std::cout << COLOUR_LINE_NR;
   for (int i = n; i < to; i++) {
      lnum_padding_out(lnum_col_max - lnum_col(i));
      std::cout << i << (i == cursor_line ? '>' : '#');
      eol_out(); }

   std::cout << COLOUR_BOTTOM;
   show_keywords();
   show_rot13();
   std::cout << COLOUR_NORMAL;
}

void
View::cursor_move_word_next(int (*f)(int))
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >= buf_->num_of_lines()) return;

   Str s { buf_->get_line(line) };
   const int len = s.len();

   for (int i = cursor_column_; i < len; i++)
      if (!f(s[i]) && f(s[i + 1])) {
         cursor_column_ = i + 1;
         break; }
}

void
View::cursor_move_word_prev(int (*f)(int))
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >= buf_->num_of_lines()) return;

   Str s { buf_->get_line(line) };
   const int len = s.len();

   for (int i = min(cursor_column_, len) - 1; i > 0; i--)
      if (!f(s[i - 1]) && f(s[i])) {
         cursor_column_ = i;
         return; }
   if (cursor_column_ > 0 && f(s[0])) // special case
      cursor_column_ = 0;
}

void
View::cursor_move_para_next()
{
   const int line = max(0, window_offset_ + cursor_row_ + 1);

   for (int i = line; i < buf_->num_of_lines(); i++) {
      if (!*buf_->get_line(i) &&
           *buf_->get_line(i + 1)) {
         cursor_row_ = i + 1 - window_offset_;
         break; }
   }
}

void
View::cursor_move_para_prev()
{
   const int line = min(buf_->num_of_lines() - 1,
                        window_offset_ + cursor_row_ - 1);

   for (int i = line; i >= 0; i--) {
      if ((!i || !*buf_->get_line(i - 1)) &&
           *buf_->get_line(i)) {
         cursor_row_ = i - window_offset_;
         break; }
   }
}

void
View::keyword_search_next()
{
   std::vector<const char *> v;
   const int from = window_offset_ + cursor_row_;
   const int to   = window_offset_ + window_height_;
   buf_->show(v, from, to);

   for (int i = 0; i < to - from; i++)
   {
      if (i == v.size()) return;
      Str s { v[i] };
      int d = i ? 0 : cursor_column_ + 1;
      auto found = s.search_word(keywords, d);
      if (found == -1) continue;

      cursor_row_    += i;
      cursor_column_  = found;
      return; }
}

void
View::keyword_toggle()
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >= buf_->num_of_lines()) return;

   Str s { buf_->get_line(line) };
   const int len = s.len();
   if (cursor_column_ >= len) return;

   int cc = cursor_column_, i0, i1;
   if (!isalpha(s[cc])) return;
   for (int i = cc; i >= 0  && isalpha(s[i]); i--) i0 = i;
   for (int i = cc; i < len && isalpha(s[i]); i++) i1 = i + 1;
   int b0 = s.index_chars_to_bytes(i0);
   int b1 = s.index_chars_to_bytes(i1);
   int size = b1 - b0;
   char *buf = (char *)malloc(size + 1);
   memcpy(buf, &buf_->get_line(line)[b0], size);
   buf[size] = '\0';

   auto i = find_if(keywords.begin(), keywords.end(), [&buf](const char *s) {
         return strcmp(s, buf) == 0; });
   if (i == keywords.end())
      // append keyword
      keywords.push_back((const char *)buf);
   else {
      // delete keyword
      free((void *)*i);
      keywords.erase(i); }
}

int
count_indent(const char *s)
{
   int i;
   for (i = 0; s[i] == ' '; i++) ;
   return i;
}

const char *
copy_indent(int n, const char *s0)
{
   int len = strlen(s0);
   char *s = (char *)malloc(n + len + 1);
   if (!s) return s;
   memset(s, ' ', n);
   strcpy(&s[n], s0);
   return s;
}

int
parent_indent(Buf *b, int line, int n0)
{
   int n1;
   for (int line2 = line - 1; ; line2--) {
      if (line2 < 0) return -1;
      const char *s1 = b->get_line(line2);
      n1 = count_indent(s1);
      if (!s1[n1]) continue;
      if (n0 > n1) break; }
   return n1;
}

void
View::indent()
{
   int line = window_offset_ + cursor_row_;
   if (line < 1 || line >= buf_->num_of_lines()) return;
   const char *s0 = buf_->get_line(line);
   const char *s1 = buf_->get_line(line - 1);
   int n0 = count_indent(s0);
   int n1 = count_indent(s1);
   if (!s1[n1] && line >= 2) {
      s1 = buf_->get_line(line - 2);
      n1 = count_indent(s1); }
   if (n0 == n1) {
      n1 = parent_indent(buf_, line, n0);
      if (n1 < 0) return;
      n1 = n0 + n0 - n1; }

   const char *s = copy_indent(n1, &s0[n0]);
   if (!s) return;
#ifdef DISABLE_UNDO
   free((void *)s0);
#endif
   buf_->replace_line(line, s);

   cursor_column_ = n1;
}

void
View::exdent()
{
   int line = window_offset_ + cursor_row_;
   if (line < 1 || line >= buf_->num_of_lines()) return;
   const char *s0 = buf_->get_line(line);
   int n0 = count_indent(s0);
   if (!n0) return;
   int n1 = parent_indent(buf_, line, n0);
   if (n1 < 0) return;
   const char *s = copy_indent(n1, &s0[n0]);
   if (!s) return;
#ifdef DISABLE_UNDO
   free((void *)s0);
#endif
   buf_->replace_line(line, s);

   cursor_column_ = n1;
}

void
View::join()
{
   int line = window_offset_ + cursor_row_;
   if (line < 0 || line + 1 >= buf_->num_of_lines()) return;
   const char *s0 = buf_->get_line(line);
   const char *s1 = buf_->get_line(line + 1);

   if (!*s0) return buf_->delete_line(line);
   if (!*s1) return buf_->delete_line(line + 1);

   const int len = strlen(s0) + strlen(s1);
   char *s = (char *)malloc(len + 1);
   if (!s) return;

   strcpy(s, s0);
   strcat(s, s1);
#ifdef DISABLE_UNDO
   free((void *)s0);
#endif
   buf_->replace_line(line, s);
   buf_->delete_line(line + 1);
}

void
View::duplicate_line()
{
   int line = window_offset_ + cursor_row_;
   if (line < 0 || line >= buf_->num_of_lines()) return;
   const char *s0 = buf_->get_line(line);
   const char *s1 = strdup(s0);
   if (!s1) return;
   buf_->insert_empty_line(line);
   const char *s2 = buf_->get_line(line);
   buf_->replace_line(line, s1);
#ifdef DISABLE_UNDO
   free((void *)s2);
#endif
}

void
View::transpose_lines()
{
   int line = window_offset_ + cursor_row_;
   if (line < 0 || line + 1 >= buf_->num_of_lines()) return;
   buf_->rotate_lines(line, /*range=*/2, /*dist=*/1);
}

// +-0-+-1-+-2-+      +-0-+-1-+-2-+
// | A | B |   |  =>  | B | A |   |
// 0---1---2---+      0---1---2---+
//           ^                  ^
void
View::transpose_chars()
{
   int line = window_offset_ + cursor_row_;
   if (line < 0 || line >= buf_->num_of_lines()) return;
   const char *s0 = buf_->get_line(line);
   if (!s0) return;
   char *s1 = strdup(s0);
   if (!s1) return;
   Str s { s1 };
   int cc = cursor_column_;
   if (cc > s.len()) cc = s.len();
   if (cc < 2) return;
   int offset0 = s.index_chars_to_bytes(cc - 2);
   int offset1 = s.index_chars_to_bytes(cc - 1);
   int offset2 = s.index_chars_to_bytes(cc);
   if (offset1 - offset0 == 1 && offset2 - offset1 == 1) {
      auto t = s1[offset0];
      s1[offset0] = s1[offset1];
      s1[offset1] = t; }
   else { /* TODO non-ascii char */ }
#ifdef DISABLE_UNDO
   free((void *)s0);
#endif
   buf_->replace_line(line, s1);
}

void
View::new_line()
{
   buf_->insert_empty_line(window_offset_ + cursor_row_++);
}

void
View::insert_new_line(bool left)
{
   int line = window_offset_ + cursor_row_;
   if (line < 0 || line >=buf_->num_of_lines()) return;
   char *s0 = (char *)buf_->get_line(line);

   if (!*s0) return new_line();

   Str s { s0 };
   const int len = s.len();
   const int index = s.index_chars_to_bytes(cursor_column_);
   const char *t, *b; // top, bottom
   b = strdup(&s0[index]);
   s0[index] = '\0';
   t = strdup(s0);

#ifdef DISABLE_UNDO
   free((void *)s0);
#endif
   if (left) {
      cursor_row_++;
      cursor_column_ = 0; }
   buf_->insert_empty_line(line);
   buf_->replace_line(line++, t);
   buf_->replace_line(line,   b);
}

void
View::char_insert(char c)
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line > buf_->num_of_lines()) return;
   if (line == buf_->num_of_lines())
      buf_->insert_empty_line(line);

   const char *s0 = buf_->get_line(line);
   Str s { s0 };
   const int size = s.size();
   char *s1 = (char *)malloc(size + 2); // \0, c
   if (!s1) return;

   const int len = s.len();
   if (cursor_column_ > len) cursor_column_ = len;

   const int index = s.index_chars_to_bytes(cursor_column_);
   strcpy(s1, s0);
   s1[index] = c;
   strcpy(s1 + index + 1, s0 + index);

   cursor_column_++;
#ifdef DISABLE_UNDO
   free((void *)s0);
#endif
   buf_->replace_line(line, s1);
}

void
View::char_delete_forward()
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >=buf_->num_of_lines()) return;

   const char *s0 = buf_->get_line(line);
   Str s { s0 };
   const int size = s.size();
   char *s1 = (char *)malloc(size);
   if (!s1) return;

   const int len = s.len();
   if (cursor_column_ >= len) {
      cursor_move_char_end();
      return join(); }

   const int index0 = s.index_chars_to_bytes(cursor_column_);
   const int index1 = s.index_chars_to_bytes(cursor_column_ + 1);
   strncpy(s1, s0, index0);
   strcpy(s1 + index0, s0 + index1);

#ifdef DISABLE_UNDO
   free((void *)s0);
#endif
   buf_->replace_line(line, s1);
}

void
View::char_delete_backward()
{
   if (cursor_column_--)
      return char_delete_forward();
   if (cursor_row_-- > 0) {
      cursor_move_char_end();
      return join(); }
   else
      cursor_column_ = cursor_row_ = 0;
}

void
View::char_delete_to_eol()
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >=buf_->num_of_lines()) return;

   const char *s0 = buf_->get_line(line);
   Str s { s0 };
   const int len = s.len();

   if (cursor_column_ >= len) { return; /* do not join */ }

   const int index = s.index_chars_to_bytes(cursor_column_);
   char *s1 = (char *)malloc(index + 1);
   if (!s1) return;
   strncpy(s1, s0, index);
   s1[index] = '\0';

#ifdef DISABLE_UNDO
   free((void *)s0);
#endif
   buf_->replace_line(line, s1);
}

void
View::char_delete_to_bol()
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >=buf_->num_of_lines()) return;

   const char *s0 = buf_->get_line(line);
   Str s { s0 };
   const int len = s.len();
   const int cc = min(cursor_column_, len);
   const int index = s.index_chars_to_bytes(cc);
   char *s1 = strdup(&s0[index]);
   if (!s1) return;
   cursor_column_ = 0;

#ifdef DISABLE_UNDO
   free((void *)s0);
#endif
   buf_->replace_line(line, s1);
}

void
View::char_rotate_variant()
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >=buf_->num_of_lines()) return;

   const char *s0 = buf_->get_line(line);
   Str s { s0 };
   if (cursor_column_ < 0 || cursor_column_ >= s.len()) return;

   const int index0 = s.index_chars_to_bytes(cursor_column_);
   const int index1 = s.index_chars_to_bytes(cursor_column_ + 1);
   const int len = index1 - index0;
   char *from = (char *)malloc(len + 1);
   if (!from) return;
   strncpy(from, &s0[index0], len);
   from[len] = '\0';

#include "rottable.h"
   const char *to = strstr(table_rotate_variant, from);
   if (!to) return;
   Str t { to };
   const int index2 = t.index_chars_to_bytes(1);
   const int index3 = t.index_chars_to_bytes(2);
   const int len2 = index3 - index2;
   const char *to2 = &to[t.index_chars_to_bytes(1)];

   const int cc = cursor_column_;
   char_delete_forward();
   for (int i = 0; i < len2; i++)
      char_insert(to2[i]);
   cursor_column_ = cc;
}

} // namespace
