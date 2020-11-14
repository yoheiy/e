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

Buf::Buf(const char *filename) : dirty_(false), new_file_(false), undo_pos_(0)
{
   load(filename);
}

void
Buf::load(const char *filename)
{
   filename_.push_back(strdup(filename));

   FILE *f = fopen(filename, "r");
   if (!f) {
      new_file_ = true;
      filesize_.push_back(0);
      return; }

   int n = 0;
   char b[160];
   while (fgets(b, sizeof b, f)) {
      n++;
      chop(b);
      lines.push_back(strdup(b)); }

   filesize_.push_back(n);
}

void
Buf::save()
{
   for (int i = 0; i < filename_.size(); i++) {
      FILE *f = fopen(filename_[i], "w");
      if (!f) continue;

      for (int j = file_pos(i); j < file_pos(i + 1); j++) {
         fputs(lines[j], f);
         fputc('\n', f); }
      if (fclose(f) == EOF) continue; }
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
   undo_push();

   auto i = lines.begin() + n;
   // free((void *)*i);

   lines.erase(i);
   filesize_[pos_file(n)]--;
   dirty_ = true;
}

void
Buf::insert_empty_line(int n)
{
   undo_push();

   auto i = lines.begin() + n;
   auto s = strdup("");

   lines.insert(i, s);
   filesize_[pos_file(n)]++;
   dirty_ = true;
}

void
Buf::replace_line(int n, const char* s)
{
   undo_push();

   lines.at(n) = s;
   dirty_ = true;
}

void
Buf::rotate_lines(int n, int range, int dist)
{
   undo_push();

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
Buf::filename(int n)
{
   return filename_[n];
}

const char *
Buf::filename_of_line(int n)
{
   return filename_[pos_file(n)];
}

int
Buf::line_length(int n)
{
   return n < 0 || n >= lines.size() ? 0 : Str(lines[n]).len();
}

int
Buf::file_pos(int n)
{
   int r = 0;
   for (int i = 0; i < n; i++)
      r += filesize_[i];
   return r;
}

int
Buf::pos_file(int n)
{
   int r = 0;
   for (int i = 0; i < filesize_.size(); i++) {
      r += filesize_[i];
      if (n < r) return i; }
   return 0; // error
}

void
Buf::undo_push()
{
   // if (!undo_buf_.empty() && undo_buf_.back() == lines) return;
   if (!undo_buf_.empty() && undo_buf_[undo_pos_] == lines) return;

   undo_buf_.push_back(lines);
   undo_pos_ = undo_buf_.size() - 1;
}

void
Buf::undo_pop(int n)
{
   undo_push();

   int p = undo_pos_ + n;
   if (p < 0 || p >= undo_buf_.size()) return;

   lines = undo_buf_[p];
   undo_pos_ = p;
}

} // namespace
