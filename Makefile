HAVE_LIBUNWIND=1

ifeq ($(HAVE_LIBUNWIND), 1)
	optional_libs=-lunwind
	BUILD_OPTIONS+=-DHAVE_LIBUNWIND
else
	optional_libs=
endif

CFLAGS=`pkg-config --cflags glib-2.0`
LIBS=`pkg-config --libs glib-2.0` $(optional_libs)

libgobject-list.so: gobject-list.c
ifeq ($(HAVE_LIBUNWIND), 1)
	@echo "Building with backtrace support (libunwind)"
else
	@echo "Building without backtrace support (libunwind disabled)"
endif
	gcc -fPIC -rdynamic -g -c -Wall ${CFLAGS} ${BUILD_OPTIONS} $<
	gcc -shared -Wl,-soname,$@ -o $@ gobject-list.o -lc -ldl ${LIBS}

libglib-backtrace.so: glib-backtrace.c
ifeq ($(HAVE_LIBUNWIND), 1)
	@echo "Building with backtrace support (libunwind)"
else
	@echo "Building without backtrace support (libunwind disabled)"
endif
	gcc -fPIC -rdynamic -g -c -Wall ${CFLAGS} ${BUILD_OPTIONS} $<
	gcc -shared -Wl,-soname,$@ -o $@ glib-backtrace.o -lc -ldl ${LIBS}

all: libglib-backtrace.so libgobject-list.so
	@echo "Done."
