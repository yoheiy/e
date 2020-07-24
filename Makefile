o=e

all: $o

$o: e.cc tc.o -ltermcap
	$(CXX) -o $@ $^

$o: rottable.h

clean:
	$(RM) $o *.o
