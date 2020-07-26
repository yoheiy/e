#include <iostream>
#include <cstring>
#include <cctype>
#include "str.h"

namespace e {

int
Str::char_size(const char *c)
{
   for (int i = 0; i < 8; i++)
      if (!((*c << i) & 0x80))
         return i;
   return -1;
}

int
Str::operator[](int index)
{
   auto s = &s_[index_chars_to_bytes(index)];
   int l, r;

   switch (l = char_size(s)) {
   case 0:
      return *s;
   case 2 ... 6:
      r = (0x7f >> l) & *s;
      for (int i = 1; i < l; i++)
         r = (r << 6) | (s[i] & 0x3f);
      return r;
   default:
      return -1; }
}

int
Str::len()
{
   int l = 0;
   for (const char *i = s_; *i; i++)
      if ((*i & 0xc0) != 0x80) l++;
   return l;
}

int
Str::index_chars_to_bytes(int n)
{
   int l = 0;
   for (const char *i = s_; *i; i++)
      if ((*i & 0xc0) != 0x80 && l++ == n)
         return i - s_;
   return size();
}

int
Str::index_bytes_to_chars(int n)
{
   int l = 0;
   const char *i = s_;
   for (; n; i++, n--)
      if ((*i & 0xc0) != 0x80) l++;
   return l;
}

int
my_strncmp(const char *s0, const char *s1, int n)
{
   int r = strncmp(s0, s1, n);
   return r ?: s1[n];
}

int
Str::search_word(std::vector<const char *> &ws, int pos=0)
{
   const char *s0 = nullptr; // word
   const char *s1 = s_;      // BOL
   const char *s2 = s1;      // search starting pos

   if (pos) {
      int d = index_chars_to_bytes(pos);
      if (d > size()) return -1;
      s2 += d; }

   for (auto j = s2; *j; j++) {
      if (!s0 && isalpha(*j)) s0 = j;
      if (s0 && !isalpha(*j)) {
         for (auto w : ws)
            if (my_strncmp(s0, w, j - s0) == 0)
               return index_bytes_to_chars(s0 - s1);
         s0 = nullptr; } }
   if (s0)
      for (auto w : ws)
         if (strcmp(s0, w) == 0)
            return index_bytes_to_chars(s0 - s1);
   return -1;
}

const char *
Str::match_word(std::vector<const char *> &ws, int pos=0)
{
   const char *s = s_;

   if (pos) {
      int d = index_chars_to_bytes(pos);
      if (d > size()) return nullptr;
      s += d; }

   const char *j;
   for (j = s; *j && isalpha(*j); j++) ;

   for (auto w : ws)
      if (my_strncmp(s, w, j - s) == 0)
         return w;

   return nullptr;
}

void
Str::output_char(int n)
{
   int x = index_chars_to_bytes(n);
   int l = char_size(&s_[x]) ?: 1;
   for (int i = 0; i < l; i++)
      std::cout << s_[x + i];
}

} // namespace
