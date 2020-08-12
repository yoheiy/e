#ifndef buf_h
#define buf_h

#include <vector>

namespace e {

class Buf {
public:
   Buf(const char *filename);
   void load(const char *filename);
   void save();
   bool dirty()    { return dirty_; }
   bool new_file() { return new_file_; }
   void show(std::vector<const char *> &v, int from, int to);

   void delete_line(int n);
   void insert_empty_line(int n);
   void replace_line(int n, const char* s);
   void rotate_lines(int n, int range, int dist);

   int num_of_lines() { return lines.size(); }
   int line_length(int n);
   const char *filename(int n = 0);
   const char *filename_of_line(int n);
   const char *get_line(int n);
private:
   int file_pos(int n);
   int pos_file(int n);
   std::vector<const char *> lines;
   std::vector<const char *> filename_;
   std::vector<int> filesize_;
   bool dirty_;
   bool new_file_;
};

} // namespace

#endif
