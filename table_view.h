#ifndef table_view_h
#define table_view_h

#include "buf.h"
#include "view.h"

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
