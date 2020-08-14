#ifndef isearch_view_h
#define isearch_view_h

#include "buf.h"
#include "filter_view.h"

namespace e {

class ISearchView : public FilterView {
public:
   ISearchView(Buf *b);
   virtual void show();
   virtual void mode_line();
   virtual int current_line();
   virtual void set_current_line(int l);

   virtual void char_insert(char c);
   virtual void char_delete_backward();
   virtual void char_delete_to_bol();

   // TODO
   virtual void page_down() { }
   virtual void page_up()   { }
   virtual void window_centre_cursor() { }
   virtual void window_move(int d) { }
   virtual void window_top()       { }
   virtual void window_bottom()    { }
   virtual void set_window_height(int n) { }

private:
   char pat_[80];
};

} // namespace

#endif
