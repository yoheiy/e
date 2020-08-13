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
#include "buf.h"
#include "view.h"
#include "table_view.h"
#include "para_view.h"
#include "isearch_view.h"
#include "app.h"

extern "C" {
   int tc_init();
   int tc(const char *);
}

namespace e {

#define ESC '\033'
void
App::mainloop()
{
   Buf  &b = *buf_;
   View &v = *(type_ == 1 ? new TableView(&b) :
               type_ == 2 ? new ParaView(&b) :
               type_ == 3 ? new ISearchView(&b) :
                            new View(&b));

   char cmd = '\0', prev_cmd;

   v.set_current_line(line_);
   if (line_) v.window_centre_cursor();
   tc("cl");
   v.show();

   while (prev_cmd = cmd, cmd = getchar(), cmd != EOF) {
      if (cmd >= ' ' && prev_cmd != ESC) {
         v.char_insert(cmd);
         tc("ho");
         v.show();
         tc("cd");
         continue; }

      if (prev_cmd == ESC) {
         switch (cmd) {
      case '#': type_ = type_ != 1 ? 1 : 0;
         line_ = v.current_line();
         delete &v; return;
      case '$': type_ = type_ != 2 ? 2 : 0;
         line_ = v.current_line();
         delete &v; return;
      case '%': type_ = type_ != 3 ? 3 : 0;
         line_ = v.current_line();
         delete &v; return;
      case '<': v.window_top();     break;
      case '>': v.window_bottom();  break;
      case 'j': v.join(); break;
      case 'd': v.duplicate_line(); break;
      case 't': v.transpose_lines(); v.cursor_move_row_rel(+1); break;
      case 'T': v.cursor_move_row_rel(-1); v.transpose_lines(); break;
      case 'f': v.cursor_move_word_next(isalpha);  break;
      case 'b': v.cursor_move_word_prev(isalpha);  break;
      case 'F': v.cursor_move_word_next(isgraph);  break;
      case 'B': v.cursor_move_word_prev(isgraph);  break;
      case 'h': v.cursor_move_row_abs(0); break;
      case 'l': v.cursor_move_row_end();  break;
      case ']': v.cursor_move_para_next();  break;
      case '[': v.cursor_move_para_prev();  break;
      case 'v': v.page_up();   break;
      case '+': v.set_window_height(v.get_window_height() + 1); break;
      case '-': v.set_window_height(v.get_window_height() - 1); break;
      case 's': b.save(); break;
      case 'k': v.keyword_toggle(); break;
      case 'n': v.keyword_search_next(); break;
      case 'p': v.keyword_search_prev(); break;
      case 'r': v.char_rotate_variant(); break;
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
      case 'Y': v.insert_new_line(false); break;
      case 'T': v.transpose_chars(); break;
      case 'V': v.page_down(); break;
      case 'X': type_ = -1; tc("cl"); return;
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

   tc("ti"); // alternative screen begin

   while (type_ >= 0)
      mainloop();

   tc("te"); // alternative screen end

out:
   /* canonical mode */
   if (tcsetattr(fd, TCSANOW, &ti_orig) == -1) {
      perror("tcsetattr (canon)"); return; }
   if (close(fd) == -1) perror("close");
}

App::App(char **a)
{
   line_ = 0;
   type_ = 0;

   buf_ = new Buf(*++a ? *a : "e.txt");
   if (!*a) return;

   for (a++; *a; a++)
      buf_->load(*a);
}

App::~App()
{
   delete buf_;
}

} // namespace
