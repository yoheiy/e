o  = e.o
o += str.o
o += buf.o
o += view.o
o += table_view.o
o += para_view.o
o += isearch_view.o
o += block_view.o
o += app.o
o += tc.o

vpath %.cc src
vpath %.c src
vpath %.h src

all: e

e: $o -ltermcap
	$(CXX) -o $@ $^

e.o: app.h
app.o: buf.h view.h app.h filter_view.h
app.o: table_view.h para_view.h isearch_view.h block_view.h
buf.o: str.h buf.h
str.o: str.h
view.o:         str.h buf.h view.h colour.h rottable.h
table_view.o:   str.h buf.h view.h colour.h table_view.h
para_view.o:    str.h buf.h view.h colour.h filter_view.h para_view.h
isearch_view.o: str.h buf.h view.h colour.h filter_view.h isearch_view.h
block_view.o:   str.h buf.h view.h colour.h filter_view.h block_view.h

clean:
	$(RM) $o *.o
