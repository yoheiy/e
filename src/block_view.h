#ifndef block_view_h
#define block_view_h

#include "buf.h"
#include "filter_view.h"

namespace e {

class BlockView : public FilterView {
public:
   BlockView(Buf *b) : FilterView(b) { }
   virtual void show();
   virtual void mode_line();
   virtual int current_line();
   virtual void set_current_line(int l);
   virtual void cursor_move_char_abs(int n);
   virtual void cursor_move_char_rel(int n);

   // TODO
   virtual void page_down() { }
   virtual void page_up()   { }
   virtual void window_centre_cursor() { }
   virtual void window_move(int d) { }
   virtual void window_top()       { }
   virtual void window_bottom()    { }
   virtual void set_window_height(int n) { }
};

} // namespace

#endif
