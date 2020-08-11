o=e

vpath %.cc src
vpath %.c src
vpath %.h src

all: $o

$o: e.o str.o buf.o view.o table_view.o para_view.o app.o tc.o -ltermcap
	$(CXX) -o $@ $^

e.o: app.h
app.o: buf.h view.h table_view.h para_view.h app.h
buf.o: str.h buf.h
str.o: str.h
view.o: str.h buf.h view.h rottable.h
para_view.o: str.h buf.h view.h para_view.h
table_view.o: str.h buf.h view.h table_view.h

clean:
	$(RM) $o *.o
