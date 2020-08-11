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

Buf::Buf(const char *filename) : dirty_(false), new_file_(false)
{
   filename_ = strdup(filename);

   FILE *f = fopen(filename, "r");
   if (!f) {
      new_file_ = true;
      return; }

   char b[160];
   while (fgets(b, sizeof b, f)) {
      chop(b);
      lines.push_back(strdup(b)); }
}

void
Buf::save()
{
   FILE *f = fopen(filename_, "w");
   if (!f) return;

   for (auto i : lines) {
      fputs(i,    f);
      fputc('\n', f); }
   if (fclose(f) == EOF) return;
   dirty_ = new_file_ = false;
}

void
Buf::show(std::vector<const char *> &v, int from, int to)
{
   int n = 0;
   for (auto i : lines) {
      if (n >= from && n < to) {
         v.push_back(i); }
      ++n; }
}

void
Buf::delete_line(int n)
{
   auto i = lines.begin() + n;
   free((void *)*i);

   lines.erase(i);
   dirty_ = true;
}

void
Buf::insert_empty_line(int n)
{
   auto i = lines.begin() + n;
   auto s = strdup("");

   lines.insert(i, s);
   dirty_ = true;
}

void
Buf::replace_line(int n, const char* s)
{
   lines.at(n) = s;
   dirty_ = true;
}

void
Buf::rotate_lines(int n, int range, int dist)
{
   if (lines.size() < n + range) return;
   while (dist < 0) dist += range;

   for (int i = 0; i < dist; i++) {
      const char *t = lines[n];
      for (int j = 0; j < range - 1; j++)
         lines[n + j] = lines[n + j + 1];
      lines[n + range - 1] = t; }
   dirty_ = true;
}

const char *
Buf::get_line(int n)
{
   return lines.at(n);
}

const char *
Buf::filename()
{
   return filename_;
}

int
Buf::line_length(int n)
{
   return n < 0 || n >= lines.size() ? 0 : Str(lines[n]).len();
}

} // namespace
