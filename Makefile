o=e

all: $o

$o: e.cc tc.o -ltermcap

clean:
	$(RM) $o *.o
