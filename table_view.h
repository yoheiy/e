#ifndef table_view_h
#define table_view_h

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

extern "C" {
   int tc_init();
   int tc(const char *);
}

namespace e {

class TableView : public View {
public:
   TableView(Buf *b) : View(b) { }
   void show();
   void cursor_move_row_rel(int n);
   void cursor_move_word_next(int (*f)(int));
   void cursor_move_word_prev(int (*f)(int));
};

} // namespace

#endif
