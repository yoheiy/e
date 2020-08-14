#ifndef filter_view_h
#define filter_view_h

#include "buf.h"
#include "view.h"

namespace e {

class FilterView : public View {
public:
   FilterView(Buf *b) : View(b) { }
   virtual void show() = 0;
   virtual void mode_line() { }

   virtual void cursor_move_row_rel(int n) { cursor_row_ += n; }
   virtual int current_line() { return 0; }
   virtual void set_current_line(int l) { }

   virtual void duplicate_line()  { }
   virtual void transpose_lines() { }

   virtual void page_down() { }
   virtual void page_up()   { }

   virtual void window_centre_cursor() { }
   virtual void window_move(int d) { }
   virtual void window_top()       { }
   virtual void window_bottom()    { }
   virtual void set_window_height(int n) { }

   // disable other methods
   virtual void cursor_move_char_abs(int n) { }
   virtual void cursor_move_char_rel(int n) { }
   virtual void cursor_move_char_end()      { }
   virtual void cursor_move_word_next(int (*f)(int)) { }
   virtual void cursor_move_word_prev(int (*f)(int)) { }
   virtual void cursor_move_para_next() { }
   virtual void cursor_move_para_prev() { }

   virtual void keyword_search_next() { }
   virtual void keyword_search_prev() { }
   virtual void keyword_toggle() { }
   virtual void show_keywords() { }
   virtual void show_rot13() { }
   virtual void indent() { }
   virtual void exdent() { }
   virtual void join() { }
   virtual void transpose_chars() { }
   virtual void new_line() { }
   virtual void insert_new_line(bool left) { }
   virtual void char_insert(char c) { }
   virtual void char_delete_forward() { }
   virtual void char_delete_backward() { }
   virtual void char_delete_to_eol() { }
   virtual void char_delete_to_bol() { }
   virtual void char_rotate_variant() { }
};

} // namespace

#endif
