LIBXML_LIBS=$(shell xml2-config --libs)
LIBXML_CFLAGS=$(shell xml2-config --cflags)
CFLAGS=$(LIBXML_CFLAGS) \
	-O0 \
	-g -ggdb \
	-pedantic \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wrestrict \
	-Wformat=2 \
	-Wwrite-strings \
	-Wcast-qual \
	-Wpointer-arith \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers


LIBOBJS = \
	cstring.o \
	htable.o \
	json.o \
	util.o \
	xml2json.o

all: clean xml2json exh

Makefile.dep:
	gcc -MM *.c > Makefile.dep 2> /dev/null || true

-include Makefile.dep

%.o : %.c
	gcc $(CFLAGS) -c $<

xml2json: $(LIBOBJS)
	gcc $(LIBXML_LIBS) -o xml2json $(LIBOBJS)

exh: htable.o util.o
	gcc $(CFLAGS) -c exh.c
	gcc -o exh htable.o util.o exh.o

check-syntax:
	gcc $(CFLAGS) -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)

clean:
	rm -f *.o Makefile.dep xml2json exh

.PHONY: all clean check-syntax
