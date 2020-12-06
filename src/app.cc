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
#include "block_view.h"
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
   View &v = *(tmp_mode_ == 1 ? new TableView(&b) :
               tmp_mode_ == 2 ? new ParaView(&b) :
               tmp_mode_ == 3 ? new ISearchView(&b) :
               tmp_mode_ == 4 ? new BlockView(&b) :
                            new View(&b));

   int cmd = '\0', prev_cmd;

   v.set_cursor_row(crow_);
   v.set_current_line(line_);
   tc("cl");
   v.show();

   while (prev_cmd = cmd, cmd = getchar(), cmd != EOF) {
      if (cmd >= ' ' && prev_cmd != ESC) {
         v.char_insert(cmd);
         tc("ho");
         v.show();
         tc("cd");
         continue; }

#undef CTRL
#define CTRL(cmd) (cmd & ((1 << 5) - 1))
#define META(cmd) (cmd | (1 << 7))

      if (prev_cmd == ESC)
         cmd = META(cmd);

      switch (cmd) {
      case META('@'): tmp_mode_ = cur_mode_ = 0; goto change_view;
      case META('#'): tmp_mode_ = cur_mode_ = 1; goto change_view;
      case META('$'): tmp_mode_ = 2; goto change_view;
      case META('%'): tmp_mode_ = 3; goto change_view;
      case CTRL('s'): tmp_mode_ = 3; goto change_view;
      case META('^'): tmp_mode_ = 4;
change_view:
         line_ = v.current_line();
         crow_ = v.cursor_row();
         delete &v; return;
      case META('<'): v.window_top();     break;
      case META('>'): v.window_bottom();  break;
      case META('j'): v.join(); break;
      case META('d'): v.duplicate_line(); break;
      case META('t'): v.transpose_lines(); v.cursor_move_row_rel(+1); break;
      case META('T'): v.cursor_move_row_rel(-1); v.transpose_lines(); break;
      case META('f'): v.cursor_move_word_next(isalpha);  break;
      case META('b'): v.cursor_move_word_prev(isalpha);  break;
      case META('F'): v.cursor_move_word_next(isgraph);  break;
      case META('B'): v.cursor_move_word_prev(isgraph);  break;
      case META('h'): v.cursor_move_row_abs(0); break;
      case META('l'): v.cursor_move_row_end();  break;
      case META(']'): v.cursor_move_para_next();  break;
      case META('['): v.cursor_move_para_prev();  break;
      case META('v'): v.page_up();   break;
      case META('+'): v.set_window_height(v.get_window_height() + 1); break;
      case META('-'): v.set_window_height(v.get_window_height() - 1); break;
      case META('{'): b.undo_pop(-1); break;
      case META('}'): b.undo_pop(+1); break;
      case META('s'): b.save(); break;
      case META('k'): v.keyword_toggle(); break;
      case META('n'): v.keyword_search_next(); break;
      case META('p'): v.keyword_search_prev(); break;
      case META('r'): v.char_rotate_variant(); break;

      case CTRL('N'): v.cursor_move_row_rel(+1);  break;
      case CTRL('P'): v.cursor_move_row_rel(-1);  break;
      case CTRL('F'): v.cursor_move_char_rel(+1); break;
      case CTRL('B'): v.cursor_move_char_rel(-1); break;
      case CTRL('A'): v.cursor_move_char_abs(0);  break;
      case CTRL('E'): v.cursor_move_char_end();   break;
      case CTRL('L'): v.window_centre_cursor();   break;
      case CTRL('I'): v.indent(); break;
      case CTRL('O'): v.exdent(); break;
      case CTRL('G'): if (tmp_mode_ != cur_mode_) {
                   tmp_mode_ = cur_mode_;
                   delete &v; return; }
      case CTRL('J'): if (tmp_mode_ != cur_mode_) {
                   tmp_mode_ = cur_mode_;
                   line_ = v.current_line();
                   crow_ = v.cursor_row();
                   v.mode_return_ok();
                   delete &v; return; }
           else v.insert_new_line(); break;
      case CTRL('Y'): v.insert_new_line(false); break;
      case CTRL('T'): v.transpose_chars(); break;
      case CTRL('V'): v.page_down(); break;
      case CTRL('X'): tmp_mode_ = -1; tc("cl"); return;
      case CTRL('D'): v.char_delete_forward();  break;
      case CTRL('H'): v.char_delete_backward(); break;
      case CTRL('K'): v.char_delete_to_eol();   break;
      case CTRL('U'): v.char_delete_to_bol();   break;
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

   while (tmp_mode_ >= 0)
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
   crow_ = 0;
   tmp_mode_ = 0;
   cur_mode_ = 0;

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
