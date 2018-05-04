#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <iostream>

extern "C" {
   int tc_init();
   int tc(const char *);
}

namespace {

const char *keywords[] = {
   "free",
   "freedom",
   "work",
};

class Str { // UTF-8 string
public:
   Str(const char *s) : s_(s) { }
   int operator[](int index) { return s_[index_chars_to_bytes(index)]; }
   int len(); // chars
   int size() { return strlen(s_); } // bytes
   int index_chars_to_bytes(int n);
   int index_bytes_to_chars(int n);
private:
   const char *s_;
};

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

class Buf {
public:
   Buf(const char *filename);
   void save();
   bool dirty() { return dirty_; }
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

   void join();
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

Buf::Buf(const char *filename) : dirty_(false)
{
   filename_ = strdup(filename);

   FILE *f = fopen(filename, "r");
   if (!f) return;

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
   dirty_ = false;
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
   tc("ce");
   lnum_padding_out(padding);
   for (int i = 1; i < 10; i++)
      printf("0    %3o", i);
   std::cout << '0';

   tc("cr");
   for (int i = 0; i < padding + col; i++)
      tc("nd");
   std::cout << '*' << std::endl;
}

int
my_strncmp(const char *s0, const char *s1, int n)
{
   int r = strncmp(s0, s1, n);
   return r ?: s1[n];
}

void
keyword_hilit(int n, const char *s)
{
   const char *s0 = nullptr;
   const char *s1 = s;
   bool found = false;

   for (auto i = s; *i; i++) {
      if (!s0 && isalpha(*i)) s0 = i;
      if (s0 && !isalpha(*i)) {
         for (auto k : keywords) {
            if (my_strncmp(s0, k, i - s0) == 0) {
               if (!found) { lnum_padding_out(n); found = true; }
               Str s { s1 };
               lnum_padding_out(s.index_bytes_to_chars(s0 - s1));
               keyword_hilit_uline(i - s0);
               s1 = i;
               break; } }
         s0 = nullptr; } }
   if (s0)
      for (auto k : keywords)
         if (strcmp(s0, k) == 0) {
            if (!found) { lnum_padding_out(n); found = true; }
            Str s { s1 };
            lnum_padding_out(s.index_bytes_to_chars(s0 - s1));
            keyword_hilit_uline(strlen(s0));
            break; }
   if (found) eol_out();
}

void
View::show()
{
   std::vector<const char *> v;
   const int from = window_offset_, to = window_offset_ + window_height_;
   const int cursor_line = window_offset_ + cursor_row_;
   const int lnum_col_max = max(lnum_col(from), lnum_col(to - 1));

   buf_->show(v, from, to);

   std::cout << "== " << buf_->filename() <<
                (buf_->dirty() ? " *" : "") <<
                " [" << from << ":" << to << "] ==";
   eol_out();

   show_ruler(lnum_col_max + 2, cursor_column_);

   for (int i = from; i < to && i < 0; i++) {
      lnum_padding_out(lnum_col_max - lnum_col(i));
      std::cout << i << '#';
      if (i == cursor_line) {
         eol_out(); }
      eol_out(); }

   int n = (from < 0) ? 0 : from;
   for (auto i : v) {
      lnum_padding_out(lnum_col_max - lnum_col(n));
      std::cout << n << ": " << i;
      eol_out();

      keyword_hilit(lnum_col_max + 2, i);

      if (n == cursor_line) {
         int m = min(cursor_column_, buf_->line_length(n));
         lnum_padding_out(lnum_col_max + 2 + m);
         std::cout << '^' << m;
         eol_out(); }
      ++n; }

   for (int i = n; i < to; i++) {
      lnum_padding_out(lnum_col_max - lnum_col(i));
      std::cout << i << '#';
      if (i == cursor_line) {
         eol_out(); }
      eol_out(); }
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
View::join()
{
   int line = window_offset_ + cursor_row_;
   const char *s0 = buf_->get_line(line);
   const char *s1 = buf_->get_line(line + 1);

   if (!*s0) return buf_->delete_line(line);
   if (!*s1) return buf_->delete_line(line + 1);

   const int len = strlen(s0) + 1 + strlen(s1);
   char *s = (char *)malloc(len + 1);
   if (!s) return;

   strcpy(s, s0);
   strcat(s, " ");
   strcat(s, s1);
   free((void *)s0);
   free((void *)s1);
   buf_->replace_line(line, s);
   buf_->delete_line(line + 1);
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
   if (cursor_column_ >= len) { return; /* TODO join */ }

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
   if (cursor_column_-- == 0) { return; /* TODO join */ }
   return char_delete_forward();
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
      case 'f': v.cursor_move_word_next();  break;
      case 'b': v.cursor_move_word_prev();  break;
      case 'h': v.cursor_move_row_abs(0); break;
      case 'l': v.cursor_move_row_end();  break;
      case 'v': v.page_up();   break;
      case '+': wh++; v.set_window_height(wh); break;
      case '-': wh--; v.set_window_height(wh); break;
      case 's': b.save(); break;
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
      case 'J': v.insert_new_line(); break;
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
