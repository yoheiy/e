#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <iostream>
#define MANUAL

extern "C" {
   int tc_init();
   int tc(const char *);
}

namespace {

class Buf {
public:
   Buf(const char *filename);
   void save() { }
   void dirty() { }
   void show(std::vector<const char *> &v, int from, int to);

   void insert_empty_line(int n);
   void replace_line(int n, const char* s);

   int num_of_lines() { return lines.size(); }
   int line_length(int n);
   const char *filename();
   const char *get_line(int n);
private:
   const char *filename_;
   std::vector<const char *> lines;
};

class View {
public:
   View(Buf * buf);
   void show();
   void page_down() { window_offset_ += window_height_; }
   void page_up()   { window_offset_ -= window_height_; }
   void window_move(int d) { window_offset_ = d; }
   void window_top()       { window_offset_ = 0; }
   void window_bottom()    { window_offset_ = buf_->num_of_lines(); }
   void set_window_height(int n) { window_height_ = n; }

   void cursor_move_row_abs(int n)  { cursor_row_     = n; }
   void cursor_move_row_rel(int n)  { cursor_row_    += n; }
   void cursor_move_row_end()       { cursor_row_     = window_height_ - 1; }
   void cursor_move_char_abs(int n) { cursor_column_  = n; }
   void cursor_move_char_rel(int n) { cursor_column_ += n; }
   void cursor_move_char_end()      { cursor_column_  =
         buf_->line_length(window_offset_ + cursor_row_) - 1; }

   void new_line();
   void char_insert(char c);
   void char_delete_forward();
   void char_delete_backward();
private:
   Buf *buf_;
   int  window_offset_;
   int  window_height_;
   int  cursor_row_;
   int  cursor_column_;
};

class App {
public:
   void mainloop();
};

Buf::Buf(const char *filename)
{
   filename_ = strdup(filename);

   FILE *f = fopen(filename, "r");
   if (!f) return;

   char b[80];
   while (fgets(b, sizeof b, f))
      lines.push_back(strdup(b));
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
Buf::insert_empty_line(int n)
{
   auto i = lines.begin() + n;
   auto s = strdup("\n");

   lines.insert(i, s);
}

void
Buf::replace_line(int n, const char* s)
{
   lines.at(n) = s;
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
   if (n < 0) return 0;
   if (n >= lines.size()) return 0;
   return strlen(lines[n]);
}

View::View(Buf * buf) :
   buf_(buf),
   window_offset_(0),
   window_height_(4),
   cursor_row_(0),
   cursor_column_(0)
{
}

int
min(int a, int b)
{
   return (a < b) ? a : b;
}

void
View::show()
{
   std::vector<const char *> v;
   const int from = window_offset_, to = window_offset_ + window_height_;
   const int cursor_line = window_offset_ + cursor_row_;

   buf_->show(v, from, to);

   std::cout << "== " << buf_->filename() <<
                " [" << from << ":" << to << "] ==";
#ifdef MANUAL
   tc("ce");
#endif
   std::cout << std::endl;

   for (int i = from; i < to && i < 0; i++) {
      std::cout << i << '#';
#ifdef MANUAL
      tc("ce");
#endif
      std::cout << std::endl; }

   int n = (from < 0) ? 0 : from;
   for (auto i : v) {
#ifdef MANUAL
      tc("ce");
#endif
      std::cout << n << ": " << i;
      if (n == cursor_line) {
         std::cout << n << "  ";
         int m = min(cursor_column_, buf_->line_length(n) - 1);
         for (int j = 0; j < m; j++)
            std::cout << ' ';
#ifdef MANUAL
         tc("ce");
#endif
         std::cout << '^' << std::endl; }
      ++n; }
}

void
View::new_line()
{
   buf_->insert_empty_line(window_offset_ + cursor_row_);
}

void
View::char_insert(char c)
{
   const int line = window_offset_ + cursor_row_;
   const char *s0 = buf_->get_line(line);
   const int len = strlen(s0);
   char *s1 = (char *)malloc(len + 2); // \0, c
   if (!s1) return;

   if (cursor_column_ >= len) cursor_column_ = len - 1;

   const int cc = cursor_column_++;
   strcpy(s1, s0);
   s1[cc] = c;
   strcpy(s1 + cc + 1, s0 + cc);

   free((void *)s0);
   buf_->replace_line(line, s1);
}

void
View::char_delete_forward()
{
   const int line = window_offset_ + cursor_row_;
   const char *s0 = buf_->get_line(line);
   const int len = strlen(s0);
   char *s1 = (char *)malloc(len);
   if (!s1) return;

   if (cursor_column_ >= len) { return; /* TODO join */ }

   const int cc = cursor_column_;
   strncpy(s1, s0, cc);
   strcpy(s1 + cc, s0 + cc + 1);

   free((void *)s0);
   buf_->replace_line(line, s1);
}

void
View::char_delete_backward()
{
   if (cursor_column_-- == 0) { return; /* TODO join */ }
   return char_delete_forward();
}

#ifdef MANUAL
void
mainloop()
{
   Buf  b("/usr/share/common-licenses/GPL-3");
   View v(&b);
   char cmd;
   int  wh = 4;

   tc("cl");
   v.show();

   while (cmd = getchar(), cmd != EOF) {
      if (cmd >= ' ') {
         v.char_insert(cmd);
         tc("ho");
         v.show();
         continue; }

      cmd += '@';

      switch (cmd) {
      case 'N': v.cursor_move_row_rel(+1);  break;
      case 'P': v.cursor_move_row_rel(-1);  break;
      case 'F': v.cursor_move_char_rel(+1); break;
      case 'B': v.cursor_move_char_rel(-1); break;
      case 'A': v.cursor_move_char_abs(0);  break;
      case 'E': v.cursor_move_char_end();   break;
      case 'J': v.new_line();  break;
      case 'G': v.page_down(); break;
      case 'T': v.page_up();   break;
//    case 'H': v.cursor_move_row_abs(0); break;
      case 'L': v.cursor_move_row_end();  break;
      case 'I': wh++; v.set_window_height(wh); break;
      case 'O': wh--; v.set_window_height(wh); break;
      case 'R': v.window_top();     break;
      case 'Y': v.window_bottom();  break;
      case 'X': tc("cl"); return;
      case 'D': v.char_delete_forward();  break;
      case 'H': v.char_delete_backward(); break;
      }
      tc("ho");
      v.show();
      tc("cd");
   }
}

void
App::mainloop()
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

   ::mainloop();

out:
   /* canonical mode */
   if (tcsetattr(fd, TCSANOW, &ti_orig) == -1) {
      perror("tcsetattr (canon)"); return; }
   if (close(fd) == -1) perror("close");
}
#else // MANUAL
void
App::mainloop()
{
   Buf  b0("spam.txt");
   Buf  b1("eggs.txt");
   Buf  b2("/usr/share/common-licenses/GPL-3");
   View v0(&b0);
   View v1(&b1);
   View v2(&b2);

   v0.show();
   v1.show();
   v2.show();

   std::cout << "[ move view(spam.txt):window_offset <- 2 ]" << std::endl;
   v0.window_move(2);
   v0.show();

   std::cout << "[ move view(eggs.txt):window_offset <- 2 ]" << std::endl;
   v1.window_move(2);
   v1.show();

   std::cout << "[ set view(GPL-3):window_height <- 12 ]" << std::endl;
   v2.set_window_height(12);
   v2.show();
   std::cout << "[ page down view(GPL-3) ]" << std::endl;
   v2.page_down();
   v2.show();
   std::cout << "[ page down view(GPL-3) ]" << std::endl;
   v2.page_down();
   v2.show();
   std::cout << "[ page down view(GPL-3) ]" << std::endl;
   v2.page_down();
   v2.show();

   std::cout << "[ move view(GPL-3):window_offset <- 6 ]" << std::endl;
   v2.window_move(6);
   v2.show();
   std::cout << "[ page up view(GPL-3) ]" << std::endl;
   v2.page_up();
   v2.show();

   std::cout << "[ move view(GPL-3):window_offset <- top ]" << std::endl;
   v2.window_top();
   v2.show();

   std::cout << "[ move view(GPL-3):window_offset <- bottom ]" << std::endl;
   v2.window_bottom();
   v2.show();
   std::cout << "[ page up view(GPL-3) ]" << std::endl;
   v2.page_up();
   v2.show();

   std::cout << "[ set view(GPL-3):window_height <- 4 ]" << std::endl;
   v2.set_window_height(4);
   std::cout << "[ move view(GPL-3):window_offset <- 12 ]" << std::endl;
   v2.window_move(12);
   v2.show();
   std::cout << "[ move view(GPL-3):cursor_row <- 1 ]" << std::endl;
   v2.cursor_move_row_abs(1);
   v2.show();
   std::cout << "[ move view(GPL-3):cursor_row <- 2 ]" << std::endl;
   v2.cursor_move_row_abs(2);
   v2.show();

   std::cout << "[ move view(GPL-3):window_offset <- top ]" << std::endl;
   v2.window_top();
   std::cout << "[ move view(GPL-3):cursor_row <- 3 ]" << std::endl;
   v2.cursor_move_row_abs(3);
   std::cout << "[ move view(GPL-3):cursor_char <- EOL ]" << std::endl;
   v2.cursor_move_char_end();
   v2.show();
   std::cout << "[ move view(GPL-3):cursor_row <- 2 ]" << std::endl;
   v2.cursor_move_row_abs(2);
   v2.show();
   std::cout << "[ move view(GPL-3):cursor_row <- 1 ]" << std::endl;
   v2.cursor_move_row_abs(1);
   v2.show();

   std::cout << "[ move view(spam.txt):window_offset <- top ]" << std::endl;
   v0.window_top();
   std::cout << "[ move view(spam.txt):cursor_row <- 1 ]" << std::endl;
   v0.cursor_move_row_abs(1);
   std::cout << "[ insert view(spam.txt):new_line ]" << std::endl;
   v0.new_line();
   v0.show();
   std::cout << "[ insert view(spam.txt):char_insert ]" << std::endl;
   v0.char_insert('e');
   v0.show();
   v0.char_insert('g');
   v0.show();
   v0.char_insert('g');
   v0.show();
   v0.char_insert('s');
   v0.show();
   std::cout << "[ move view(spam.txt):cursor_row <- 2 ]" << std::endl;
   v0.cursor_move_row_abs(2);
   std::cout << "[ insert view(spam.txt):char_insert ]" << std::endl;
   v0.char_insert(' ');
   v0.show();
   v0.char_insert('e');
   v0.show();
   v0.char_insert('g');
   v0.show();
   v0.char_insert('g');
   v0.show();
   v0.char_insert('s');
   v0.show();

   std::cout << "[ move view(eggs.txt):window_offset <- top ]" << std::endl;
   v1.window_top();
   std::cout << "[ move view(eggs.txt):cursor_row <- 0 ]" << std::endl;
   v1.cursor_move_row_abs(0);
   std::cout << "[ move view(eggs.txt):cursor_char <- EOL ]" << std::endl;
   v1.cursor_move_char_end();
   v1.show();
   std::cout << "[ move view(eggs.txt):cursor_row <- 1 ]" << std::endl;
   v1.cursor_move_row_abs(1);
   v1.show();
   std::cout << "[ insert view(eggs.txt):char_insert ]" << std::endl;
   v1.char_insert(' ');
   v1.char_insert('s');
   v1.char_insert('p');
   v1.char_insert('a');
   v1.char_insert('m');
   v1.show();
}
#endif // MANUAL

} // namespace

int
main()
{
   App a;

   a.mainloop();
}
