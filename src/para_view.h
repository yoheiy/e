#ifndef para_view_h
#define para_view_h

#include "buf.h"
#include "filter_view.h"

namespace e {

class ParaView : public FilterView {
public:
   ParaView(Buf *b) : FilterView(b) { }
   virtual void show();
   virtual void cursor_move_para_next() { cursor_move_row_rel(+1); }
   virtual void cursor_move_para_prev() { cursor_move_row_rel(-1); }
   virtual void mode_line();
   virtual void transpose_lines(); // transpose para
   virtual int current_line();
   virtual void set_current_line(int l);

   // TODO
   virtual void duplicate_line()  { } // duplicate para
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
