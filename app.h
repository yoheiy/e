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

namespace e {

class App {
public:
   App(char **a);
   void go();
private:
   void mainloop();

   const char *filename_;
   int line_;
   int type_;
};

} // namespace
