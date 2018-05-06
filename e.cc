#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>
#define COLOUR_ESC "\033"
#define COLOUR_RED     COLOUR_ESC "[31m"
#define COLOUR_GREY    COLOUR_ESC "[38;5;248m"
#define COLOUR_NORMAL  COLOUR_ESC "[0m"
#define COLOUR_GREY_BG COLOUR_ESC "[48;5;248m"

extern "C" {
   int tc_init();
   int tc(const char *);
}

namespace {

std::vector<const char *> keywords;

class Str { // UTF-8 string
public:
   Str(const char *s) : s_(s) { }
   int operator[](int index);
   int len(); // chars
   int size() { return strlen(s_); } // bytes
   int index_chars_to_bytes(int n);
   int index_bytes_to_chars(int n);
   int search_word(std::vector<const char *> &ws, int pos);
   const char *match_word(std::vector<const char *> &ws, int pos);
   void output_char(int n);
private:
   const char *s_;
   static int char_size(const char *c);
};

int
Str::char_size(const char *c)
{
   for (int i = 0; i < 8; i++)
      if (!((*c << i) & 0x80))
         return i;
   return -1;
}

int
Str::operator[](int index)
{
   auto s = &s_[index_chars_to_bytes(index)];
   int l, r;

   switch (l = char_size(s)) {
   case 0:
      return *s;
   case 2 ... 6:
      r = (0x7f >> l) & *s;
      for (int i = 1; i < l; i++)
         r = (r << 6) | (s[i] & 0x3f);
      return r;
   default:
      return -1; }
}

int
Str::len()
{
   int l = 0;
   for (const char *i = s_; *i; i++)
      if ((*i & 0xc0) != 0x80) l++;
   return l;
}

int
Str::index_chars_to_bytes(int n)
{
   int l = 0;
   for (const char *i = s_; *i; i++)
      if ((*i & 0xc0) != 0x80 && l++ == n)
         return i - s_;
   return size();
}

int
Str::index_bytes_to_chars(int n)
{
   int l = 0;
   const char *i = s_;
   for (; n; i++, n--)
      if ((*i & 0xc0) != 0x80) l++;
   return l;
}

int
my_strncmp(const char *s0, const char *s1, int n)
{
   int r = strncmp(s0, s1, n);
   return r ?: s1[n];
}

int
Str::search_word(std::vector<const char *> &ws, int pos=0)
{
   const char *s0 = nullptr; // word
   const char *s1 = s_;      // BOL
   const char *s2 = s1;      // search starting pos

   if (pos) {
      int d = index_chars_to_bytes(pos);
      if (d > size()) return -1;
      s2 += d; }

   for (auto j = s2; *j; j++) {
      if (!s0 && isalpha(*j)) s0 = j;
      if (s0 && !isalpha(*j)) {
         for (auto w : ws)
            if (my_strncmp(s0, w, j - s0) == 0)
               return index_bytes_to_chars(s0 - s1);
         s0 = nullptr; } }
   if (s0)
      for (auto w : ws)
         if (strcmp(s0, w) == 0)
            return index_bytes_to_chars(s0 - s1);
   return -1;
}

const char *
Str::match_word(std::vector<const char *> &ws, int pos=0)
{
   const char *s = s_;

   if (pos) {
      int d = index_chars_to_bytes(pos);
      if (d > size()) return nullptr;
      s += d; }

   const char *j;
   for (j = s; *j && isalpha(*j); j++) ;

   for (auto w : ws)
      if (my_strncmp(s, w, j - s) == 0)
         return w;

   return nullptr;
}

void
Str::output_char(int n)
{
   int x = index_chars_to_bytes(n);
   int l = char_size(&s_[x]) ?: 1;
   for (int i = 0; i < l; i++)
      std::cout << s_[x + i];
}

class Buf {
public:
   Buf(const char *filename);
   void save();
   bool dirty()    { return dirty_; }
   bool new_file() { return new_file_; }
   void show(std::vector<const char *> &v, int from, int to);

   void delete_line(int n);
   void insert_empty_line(int n);
   void replace_line(int n, const char* s);

   int num_of_lines() { return lines.size(); }
   int line_length(int n);
   const char *filename();
   const char *get_line(int n);
private:
   const char *filename_;
   std::vector<const char *> lines;
   bool dirty_;
   bool new_file_;
};

class View {
public:
   View(Buf * buf);
   void show();
   void page_down() { window_offset_ += window_height_; }
   void page_up()   { window_offset_ -= window_height_; }
   void window_move(int d) { window_offset_ = d; }
   void window_top()       { window_offset_ = 0; }
   void window_bottom()    { window_offset_ =
         buf_->num_of_lines() - window_height_; }
   void set_window_height(int n) { window_height_ = n; }

   void cursor_move_row_abs(int n)  { cursor_row_     = n; }
   void cursor_move_row_rel(int n)  { cursor_row_    += n; }
   void cursor_move_row_end()       { cursor_row_     = window_height_ - 1; }
   void cursor_move_char_abs(int n) { cursor_column_  = n; }
   void cursor_move_char_rel(int n) { cursor_column_ += n; }
   void cursor_move_char_end()      { cursor_column_  =
         buf_->line_length(window_offset_ + cursor_row_); }
   void cursor_move_word_next();
   void cursor_move_word_prev();

   void window_centre_cursor() {
         int t = window_height_ / 2;
         window_offset_ += cursor_row_ - t;
         cursor_row_ = t; }

   void keyword_search_next();
   void keyword_search_prev() { }
   void keyword_toggle();
   void show_rot13();
   void indent();
   void exdent();
   void join();
   void duplicate_line();
   void transpose_lines();
   void transpose_chars();
   void new_line();
   void insert_new_line();
   void char_insert(char c);
   void char_delete_forward();
   void char_delete_backward();
   void char_delete_to_eol();
   void char_delete_to_bol();
private:
   Buf *buf_;
   int  window_offset_;
   int  window_height_;
   int  cursor_row_;
   int  cursor_column_; // chars
};

class App {
public:
   App(char **a);
   void go();
private:
   void mainloop();

   const char *filename_;
};

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

Buf::Buf(const char *filename) : dirty_(false), new_file_(false)
{
   filename_ = strdup(filename);

   FILE *f = fopen(filename, "r");
   if (!f) {
      new_file_ = true;
      return; }

   char b[160];
   while (fgets(b, sizeof b, f)) {
      chop(b);
      lines.push_back(strdup(b)); }
}

void
Buf::save()
{
   FILE *f = fopen(filename_, "w");
   if (!f) return;

   for (auto i : lines) {
      fputs(i,    f);
      fputc('\n', f); }
   if (fclose(f) == EOF) return;
   dirty_ = new_file_ = false;
}

void
Buf::show(std::vector<const char *> &v, int from, int to)
{
   int n = 0;
   for (auto i : lines) {
      if (n >= from && n < to) {
         v.push_back(i); }
      ++n; }
}

void
Buf::delete_line(int n)
{
   auto i = lines.begin() + n;
   free((void *)*i);

   lines.erase(i);
   dirty_ = true;
}

void
Buf::insert_empty_line(int n)
{
   auto i = lines.begin() + n;
   auto s = strdup("");

   lines.insert(i, s);
   dirty_ = true;
}

void
Buf::replace_line(int n, const char* s)
{
   lines.at(n) = s;
   dirty_ = true;
}

const char *
Buf::get_line(int n)
{
   return lines.at(n);
}

const char *
Buf::filename()
{
   return filename_;
}

int
Buf::line_length(int n)
{
   return n < 0 || n >= lines.size() ? 0 : Str(lines[n]).len();
}

View::View(Buf * buf) :
   buf_(buf),
   window_offset_(0),
   window_height_(20),
   cursor_row_(0),
   cursor_column_(0)
{
}

int min(int a, int b) { return (a < b) ? a : b; }
int max(int a, int b) { return (a > b) ? a : b; }

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
show_ruler(int padding, int col)
{
   std::cout << COLOUR_GREY;
   tc("ce");
   lnum_padding_out(padding);
   for (int i = 1; i < 10; i++)
      printf("0    %3o", i);
   std::cout << '0';
   std::cout << COLOUR_NORMAL;

   tc("cr");
   for (int i = 0; i < padding + col; i++)
      tc("nd");
   std::cout << '*' << std::endl;
}

void
keyword_hilit_colour(const char *s, int col)
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
      if ((!i || buf[i - 1] != '~') && buf[i] == '~')
         std::cout << COLOUR_RED;
      if (i && buf[i - 1] == '~' && buf[i] != '~')
         std::cout << COLOUR_NORMAL;
      if (buf[i] == '^')
         std::cout << COLOUR_GREY_BG;
      str.output_char(i);
      if (buf[i] == '^')
         std::cout << COLOUR_NORMAL; }

   // EOL
   std::cout << COLOUR_NORMAL;
   if (col == len)
      std::cout << COLOUR_GREY_BG;
   else
      std::cout << COLOUR_GREY;
   std::cout << '$' << COLOUR_NORMAL;
}

void
show_keywords()
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

   const char *s { buf_->get_line(line) };
   std::cout << "r13 ";
   for (auto i = s; *i; i++) {
      auto c = *i;
      if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M')) c += 13; else
      if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z')) c -= 13;
      std::cout << c; }
   eol_out();
}

void
View::show()
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

   show_ruler(lnum_col_max + 2, cursor_column_);

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
      keyword_hilit_colour(i, n == cursor_line ? cursor_column_ : -1);
      eol_out();

      if (n == cursor_line) {
         int m = min(cursor_column_, buf_->line_length(n));
         int c = Str(i)[m];
         lnum_padding_out(lnum_col_max + 2 + m);
         std::cout << COLOUR_GREY_BG;
         std::cout << '^';
         std::cout << COLOUR_NORMAL;
         std::cout << COLOUR_GREY;
         std::cout << m;
         std::cout << '#' << std::hex << c << std::dec;
         std::cout << COLOUR_NORMAL;
         eol_out(); }
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

void
View::cursor_move_word_next()
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >= buf_->num_of_lines()) return;

   Str s { buf_->get_line(line) };
   const int len = s.len();

   int i = cursor_column_;
   for (; i < len &&  isalpha(s[i]); i++) ;
   for (; i < len && !isalpha(s[i]); i++) ;
   cursor_column_ = i;
}

void
View::cursor_move_word_prev()
{
   const int line = window_offset_ + cursor_row_;
   if (line < 0 || line >= buf_->num_of_lines()) return;

   Str s { buf_->get_line(line) };
   const int len = s.len();

   if (cursor_column_ > len) cursor_column_ = len;
   if (cursor_column_ == 0) return;

   int i = cursor_column_;
   for (; i > 0 &&  isalpha(s[i]); i--) ;
   for (; i > 0 && !isalpha(s[i]); i--) ;
   cursor_column_ = i;
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

void
View::indent()
{
   int line = window_offset_ + cursor_row_;
   if (line < 1 || line >= buf_->num_of_lines()) return;
   const char *s0 = buf_->get_line(line);
   const char *s1 = buf_->get_line(line - 1);
   int n0 = count_indent(s0);
   int n1 = count_indent(s1);
   if (n0 == n1) return;

   const char *s = copy_indent(n1, &s0[n0]);
   if (!s) return;
   free((void *)s0);
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
   int n1;
   for (int line2 = line - 1; ; line2--) {
      if (line2 < 0) return;
      const char *s1 = buf_->get_line(line2);
      if (!*s1) continue;
      n1 = count_indent(s1);
      if (n0 > n1) break; }
   const char *s = copy_indent(n1, &s0[n0]);
   if (!s) return;
   free((void *)s0);
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
   free((void *)s0);
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
   free((void *)s2);
}

void
View::transpose_lines()
{
   int line = window_offset_ + cursor_row_;
   if (line < 0 || line + 1 >= buf_->num_of_lines()) return;
   const char *s0 = buf_->get_line(line);
   const char *s1 = buf_->get_line(line + 1);

   buf_->replace_line(line,     s1);
   buf_->replace_line(line + 1, s0);
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
   free((void *)s0);
   buf_->replace_line(line, s1);
}

void
View::new_line()
{
   buf_->insert_empty_line(window_offset_ + cursor_row_++);
}

void
View::insert_new_line()
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

   free((void *)s0);
   cursor_row_++;
   cursor_column_ = 0;
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
   free((void *)s0);
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

   free((void *)s0);
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

   free((void *)s0);
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

   free((void *)s0);
   buf_->replace_line(line, s1);
}

#define ESC '\033'
void
App::mainloop()
{
   Buf  b(filename_);
   View v(&b);
   char cmd = '\0', prev_cmd;
   int  wh = 20;

   tc("cl");
   v.set_window_height(wh);
   v.show();

   while (prev_cmd = cmd, cmd = getchar(), cmd != EOF) {
      if (cmd >= ' ' && prev_cmd != ESC) {
         v.char_insert(cmd);
         tc("ho");
         v.show();
         continue; }

      if (prev_cmd == ESC) {
         switch (cmd) {
      case '<': v.window_top();     break;
      case '>': v.window_bottom();  break;
      case 'j': v.join(); break;
      case 'd': v.duplicate_line(); break;
      case 't': v.transpose_lines(); v.cursor_move_row_rel(+1); break;
      case 'T': v.cursor_move_row_rel(-1); v.transpose_lines(); break;
      case 'f': v.cursor_move_word_next();  break;
      case 'b': v.cursor_move_word_prev();  break;
      case 'h': v.cursor_move_row_abs(0); break;
      case 'l': v.cursor_move_row_end();  break;
      case 'v': v.page_up();   break;
      case '+': wh++; v.set_window_height(wh); break;
      case '-': wh--; v.set_window_height(wh); break;
      case 's': b.save(); break;
      case 'k': v.keyword_toggle(); break;
      case 'n': v.keyword_search_next(); break;
      case 'p': v.keyword_search_prev(); break;
         }
         tc("ho");
         v.show();
         tc("cd");
         continue; }

      switch (cmd + '@') {
      case 'N': v.cursor_move_row_rel(+1);  break;
      case 'P': v.cursor_move_row_rel(-1);  break;
      case 'F': v.cursor_move_char_rel(+1); break;
      case 'B': v.cursor_move_char_rel(-1); break;
      case 'A': v.cursor_move_char_abs(0);  break;
      case 'E': v.cursor_move_char_end();   break;
      case 'L': v.window_centre_cursor();   break;
      case 'I': v.indent(); break;
      case 'O': v.exdent(); break;
      case 'J': v.insert_new_line(); break;
      case 'T': v.transpose_chars(); break;
      case 'V': v.page_down(); break;
      case 'X': tc("cl"); return;
      case 'D': v.char_delete_forward();  break;
      case 'H': v.char_delete_backward(); break;
      case 'K': v.char_delete_to_eol();   break;
      case 'U': v.char_delete_to_bol();   break;
      }
      tc("ho");
      v.show();
      tc("cd");
   }
}

void
App::go()
{
   int fd;
   struct termios ti, ti_orig;
   char ibuf[1], obuf[40];

   if (fd = open("/dev/tty", O_RDWR), fd == -1) { perror("open"); return; }
   if (tcgetattr(fd, &ti) == -1) { perror("tcgetattr"); return; }
   ti_orig = ti;

   /* non-canonical mode */
   ti.c_lflag &= ~ICANON;
   ti.c_lflag &= ~ECHO;
   if (tcsetattr(fd, TCSANOW, &ti) == -1) {
      perror("tcsetattr (noncanon)"); return; }

   tc_init();

   mainloop();

out:
   /* canonical mode */
   if (tcsetattr(fd, TCSANOW, &ti_orig) == -1) {
      perror("tcsetattr (canon)"); return; }
   if (close(fd) == -1) perror("close");
}

App::App(char **a)
{
   filename_ = a[1] ?: "e.txt";
}

} // namespace

int
main(int argc, char *argv[])
{
   App a{argv};

   a.go();
}
