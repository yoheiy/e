o  = e.o
o += str.o
o += buf.o
o += view.o
o += table_view.o
o += para_view.o
o += isearch_view.o
o += app.o
o += tc.o
o += -ltermcap

vpath %.cc src
vpath %.c src
vpath %.h src

all: e

e: $o
	$(CXX) -o $@ $^

e.o: app.h
app.o: buf.h view.h table_view.h para_view.h app.h
buf.o: str.h buf.h
str.o: str.h
view.o:         str.h buf.h view.h rottable.h
para_view.o:    str.h buf.h view.h para_view.h
table_view.o:   str.h buf.h view.h table_view.h
isearch_view.o: str.h buf.h view.h isearch_view.h

clean:
	$(RM) $o *.o
