LIBXML_LIBS=$(shell xml2-config --libs)
LIBXML_CFLAGS=$(shell xml2-config --cflags)
CFLAGS=$(LIBXML_CFLAGS) \
	-std=c99 \
	-Wextra \
	-Wall -W \
	-Wno-missing-field-initializers \
	-O0 \
	-g -ggdb

LIBOBJS = xml2json.o

all: clean xml2json

Makefile.dep:
	gcc -MM *.c > Makefile.dep 2> /dev/null || true

-include Makefile.dep

%.o : %.c
	gcc $(CFLAGS) -c $<

xml2json: $(LIBOBJS)
	gcc $(LIBXML_LIBS) -o xml2json $(LIBOBJS)

check-syntax:
	gcc $(CFLAGS) -Wextra -pedantic -fsyntax-only $(CHK_SOURCES)

clean:
	rm -f *.o Makefile.dep xml2json

.PHONY: all clean check-syntax
