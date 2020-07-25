#ifndef str_h
#define str_h

#include <vector>

namespace e {

class Str { // UTF-8 string
public:
   Str(const char *s) : s_(s) { }
   int operator[](int index);
   int len(); // chars
   int size() { return strlen(s_); } // bytes
   int index_chars_to_bytes(int n);
   int index_bytes_to_chars(int n);
   int search_word(std::vector<const char *> &ws, int pos);
   const char *match_word(std::vector<const char *> &ws, int pos);
   void output_char(int n);
private:
   const char *s_;
   static int char_size(const char *c);
};

} // namespace

#endif
