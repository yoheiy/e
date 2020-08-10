#ifndef view_h
#define view_h

#include <vector>

#include "buf.h"

namespace e {

class View {
public:
   View(Buf * buf);
   virtual void show();
   virtual void page_down() { window_offset_ += window_height_; }
   virtual void page_up()   { window_offset_ -= window_height_; }
   virtual void window_move(int d) { window_offset_ = d; }
   virtual void window_top()       { window_offset_ = 0; }
   virtual void window_bottom()    { window_offset_ =
         buf_->num_of_lines() - window_height_; }
   virtual void set_window_height(int n) { window_height_ = n; }
   virtual int  get_window_height() { return window_height_; }

   virtual void cursor_move_row_abs(int n)  { cursor_row_     = n; }
   virtual void cursor_move_row_rel(int n)  { cursor_row_    += n; }
   virtual void cursor_move_row_end()       { cursor_row_     = window_height_ - 1; }
   virtual void cursor_move_char_abs(int n) { cursor_column_  = n; }
   virtual void cursor_move_char_rel(int n) { cursor_column_ += n; }
   virtual void cursor_move_char_end()      { cursor_column_  =
         buf_->line_length(window_offset_ + cursor_row_); }
   virtual void cursor_move_word_next(int (*f)(int));
   virtual void cursor_move_word_prev(int (*f)(int));
   virtual void cursor_move_para_next();
   virtual void cursor_move_para_prev();

   virtual void window_centre_cursor();

   virtual void keyword_search_next();
   virtual void keyword_search_prev() { }
   virtual void keyword_toggle();
   virtual void show_keywords();
   virtual void show_rot13();
   virtual void indent();
   virtual void exdent();
   virtual void join();
   virtual void duplicate_line();
   virtual void transpose_lines();
   virtual void transpose_chars();
   virtual void new_line();
   virtual void insert_new_line(bool left=true);
   virtual void char_insert(char c);
   virtual void char_delete_forward();
   virtual void char_delete_backward();
   virtual void char_delete_to_eol();
   virtual void char_delete_to_bol();
   virtual void char_rotate_variant();

   virtual int current_line() { return window_offset_ + cursor_row_; }
   virtual void set_current_line(int l) { cursor_row_ = l - window_offset_; }
protected:
   Buf *buf_;
   int  window_offset_;
   int  window_height_;
   int  window_width_;
   int  cursor_row_;
   int  cursor_column_; // chars

   virtual void keyword_hilit_colour(const char *s, int col, int width);
};

} // namespace

#endif
