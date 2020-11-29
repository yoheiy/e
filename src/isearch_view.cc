#include <iostream>
#include <vector>
#include <regex.h>
#define COLOUR_ESC "\033"
#define COLOUR_RED     COLOUR_ESC "[31m"
#define COLOUR_CYAN    COLOUR_ESC "[36m"
#define COLOUR_GREY    COLOUR_ESC "[38;5;248m"
#define COLOUR_NORMAL  COLOUR_ESC "[0m"
#define COLOUR_GREY_BG COLOUR_ESC "[48;5;248m"
#define COLOUR_HILIGHT COLOUR_ESC "[43m"

#include "str.h"
#include "buf.h"
#include "isearch_view.h"

namespace {

char
chop(char *s)
{
   char c;
   int  l;

   l = strlen(s);
   if (!l) return '\0';
   l--;
   c = s[l];
   s[l] = '\0';
   return c;
}

}
namespace e {

void eol_out();
int  lnum_col(int n);
void lnum_padding_out(int n);

void
show_pat_line(const char *pat)
{
   std::cout << '[' << pat;
   std::cout << COLOUR_GREY_BG;
   std::cout << ']';
   std::cout << COLOUR_NORMAL;
   eol_out();
}

ISearchView::ISearchView(Buf *b) : FilterView(b)
{
   *pat_ = '\0';
}

void
ISearchView::char_insert(char c)
{
   const int cl = current_line();

   const int l = strlen(pat_);
   if (l + 1 == sizeof pat_) return;
   pat_[l] = c;
   pat_[l + 1] = '\0';

   int n = 0;
   for (int i = 0; i < buf_->num_of_lines(); i++) {
      if (i == cl) {
         window_offset_ = n - cursor_row_;
         return; }
      if (strstr(buf_->get_line(i), pat_))
         n++; }
   window_offset_ = n - cursor_row_ - 1;
}

void
ISearchView::char_delete_backward()
{
   const int cl = current_line();

   chop(pat_);

   int n = 0;
   for (int i = 0; i < buf_->num_of_lines(); i++) {
      if (i == cl) {
         window_offset_ = n - cursor_row_;
         return; }
      if (strstr(buf_->get_line(i), pat_))
         n++; }
}

void
ISearchView::char_delete_to_bol()
{
   const int cl = current_line();

   *pat_ = '\0';

   set_current_line(cl);
}

void
ISearchView::mode_line()
{
   std::cout << COLOUR_GREY_BG;
   std::cout << "== " << buf_->filename_of_line(current_line()) <<
                (buf_->new_file() ? " N" : buf_->dirty() ? " *" : "") <<
                " [isearch] ==";
   eol_out();
   std::cout << COLOUR_NORMAL;
}

void
ISearchView::show()
{
   mode_line();
   show_pat_line(pat_);

   char *t_pat = strdup(pat_);
   char *t_sub = strchr(t_pat, '/');
   if (t_sub) { *t_sub = '\0'; t_sub++; }

   regex_t r;
   int err = regcomp(&r, t_pat, REG_EXTENDED);
   if (err) {
      char mesg[80];
      regerror(err, &r, mesg, sizeof mesg);
      std::cout << "REGEX ERR : " << mesg; eol_out();
      free(t_pat);
      regfree(&r);
      return; }

   std::vector<int> ll;
   for (int i = 0; i < buf_->num_of_lines(); i++)
      if (regexec(&r, buf_->get_line(i), 0, NULL, 0) != REG_NOMATCH)
         ll.push_back(i);

   for (int i = 0; i < window_height_; i++) {
      int x = window_offset_ + i;
      if (x < 0 || x >= ll.size()) {
         std::cout << (i == cursor_row_ ? COLOUR_GREY_BG : COLOUR_GREY);
         std::cout << '$' << COLOUR_NORMAL;
         eol_out();
         continue; }
      int k = ll[x];
      lnum_padding_out(lnum_col(buf_->num_of_lines()) - lnum_col(k));
      std::cout << (i == cursor_row_ ? COLOUR_GREY_BG : COLOUR_GREY);
      std::cout << k << ": " << COLOUR_NORMAL;

      const char *s = buf_->get_line(k);
      while (*s) {
         regmatch_t m;
         if (regexec(&r, s, 1, &m, 0) == REG_NOMATCH) break;
         const char * const p = &s[m.rm_so];
         const int l = m.rm_eo - m.rm_so;
         if (!l) { std::cout << *s++; continue; }
         for (const char *q = s; q < p; q++) std::cout << *q;
         if (!t_sub) {
            std::cout << COLOUR_HILIGHT;
            for (const char *q = p; q < &p[l]; q++) std::cout << *q; }
         else
            std::cout << COLOUR_CYAN << t_sub;
         std::cout << COLOUR_NORMAL;
         s = &p[l]; }

      for (const char *q = s; *q; q++) std::cout << *q;
      std::cout << COLOUR_GREY << '$' << COLOUR_NORMAL;
      eol_out(); }

out:
   free(t_pat);
   regfree(&r);
}

int
ISearchView::current_line()
{
   int n = 0;

   char *t_pat = strdup(pat_);
   char *t_sub = strchr(t_pat, '/');
   if (t_sub) { *t_sub = '\0'; }

   for (int i = 0; i < buf_->num_of_lines(); i++) {
      const char *s = buf_->get_line(i);
      if (strstr(s, pat_)) {
         if (n == window_offset_ + cursor_row_)
            return i;
         n++; } }

   free(t_pat);

   return 0;
   return buf_->num_of_lines();
}

void
ISearchView::set_current_line(int l)
{
   // assume pat_ == ""
   window_offset_ = l - cursor_row_;
}

void
ISearchView::mode_return_ok()
{
   if (!*pat_) return;
   char *t_pat = strdup(pat_);
   char *t_sub = strchr(t_pat, '/');
   if (t_sub) { *t_sub = '\0'; t_sub++; } else { free(t_pat); return; }
   if (!*t_pat) { free(t_pat); return; }
   const int l_pat = strlen(t_pat);
   const int l_sub = strlen(t_sub);

   regex_t r;
   int err = regcomp(&r, t_pat, REG_EXTENDED);
   if (err) { free(t_pat); regfree(&r); return; }

   for (int i = 0; i < buf_->num_of_lines(); i++) {
      const char *s = buf_->get_line(i);
      if (regexec(&r, s, 0, NULL, 0) == REG_NOMATCH) continue;

      int l_new = 1; // for NUL
      while (*s) {
         regmatch_t m;
         if (regexec(&r, s, 1, &m, 0) == REG_NOMATCH) break;
         const char * const p = &s[m.rm_so];
         const int l = m.rm_eo - m.rm_so;
         if (!l) { s++; continue; }
         l_new += p - s + l_sub;
         s = &p[l]; }
      l_new += strlen(s);

      char *t_new = (char *)malloc(l_new);
      char *t = t_new;
      s = buf_->get_line(i);
      while (*s) {
         regmatch_t m;
         if (regexec(&r, s, 1, &m, 0) == REG_NOMATCH) break;
         const char * const p = &s[m.rm_so];
         const int l = m.rm_eo - m.rm_so;
         if (!l) { s++; continue; }
         for (const char *q = s; q < p; q++) *t++ = *q;
         t = strcpy(t, t_sub) + l_sub;
         s = &p[l]; }
      l_new += strlen(s);

      strcpy(t, s);
#ifdef DISABLE_UNDO
      s = buf_->get_line(i);
      free((void *)s);
#endif
      buf_->replace_line(i, t_new); }

   regfree(&r);
   free(t_pat);
}

} // namespace
