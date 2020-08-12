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

#include "buf.h"

namespace e {

class App {
public:
   App(char **a);
   ~App();
   void go();
private:
   void mainloop();

   const char *filename_;
   int line_;
   int type_;

   Buf *buf_;
};

} // namespace
