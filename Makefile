#libxml options
LIBXML_LIBS=$(shell xml2-config --libs)
LIBXML_CFLAGS=$(shell xml2-config --cflags)

## Platform
UNAME := $(shell $(CC) -dumpmachine 2>&1 | grep -E -o "linux|darwin")

ifeq ($(UNAME), linux)
OSFLAGS = -DLINUX -D_GNU_SOURCE
DEBUG = -g -ggdb
else ifeq ($(UNAME), darwin)
OSFLAGS = -DMACOSX -D_BSD_SOURCE
DEBUG = -g
endif

CFLAGS=$(LIBXML_CFLAGS) \
	-O0 \
	$(OSFLAGS) \
	$(DEBUG) \
	-pedantic \
	-Wall \
	-Wextra \
	-Wshadow \
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
