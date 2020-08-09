o=e

vpath %.cc src
vpath %.c src
vpath %.h src

all: $o

$o: e.o str.o buf.o view.o table_view.o para_view.o app.o tc.o -ltermcap
	$(CXX) -o $@ $^

view.o: rottable.h

clean:
	$(RM) $o *.o
