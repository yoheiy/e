#include <termcap.h>
#include <stdio.h>
#include <stdlib.h>

int
tc_err(const char *s, int n)
{
   fputs(s, stderr);
   return n;
}

char buf[2048];

int
tc_init(void)
{
   char *term;

   if ((term = getenv("TERM")) == NULL) return tc_err("getenv", 1);
   if ((tgetent(buf, term)) < 0)        return tc_err("tgetent", 1);
   return 0;
}

int
tc(const char *cap)
{
   char  s[2048];
   char *p = s;

   if (tgetstr(cap, &p) < 0) return tc_err("tgetstr", 1);
   tputs(s, 1, putchar);
   return 0;
}

#if 0
int
main(int argc, char *argv[])
{
   if (argc != 2)   return 2;
   if (tc_init())   return 1;
   if (tc(argv[1])) return 1;
   return 0;
}
#endif
