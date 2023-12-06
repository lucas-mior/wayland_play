PREFIX ?= /usr/local

.PHONY: all clean install uninstall
.SUFFIXES:
.SUFFIXES: .c .o

PKG_CONFIG ?= pkg-config

clang: CC=clang
clang: CFLAGS += -Weverything -Wno-unsafe-buffer-usage
clang: clean wayland-play

release: CFLAGS += -O2 -flto
release: wayland-play

debug: CFLAGS += -g
debug: CFLAGS += -Dwayland-play_DEBUG -fsanitize=undefined
debug: clean wayland-play

CFLAGS += -std=c99 -D_DEFAULT_SOURCE
CFLAGS += -Wall -Wextra

WAYLAND_FLAGS = $(shell $(PKG_CONFIG) wayland-client --cflags --libs)
WAYLAND_PROTOCOLS_DIR = $(shell $(PKG_CONFIG) wayland-protocols --variable=pkgdatadir)

WAYLAND_SCANNER = $(shell pkg-config --variable=wayland_scanner wayland-scanner)

XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml

OBJS = xdg-shell-protocol.o
HEADERS = xdg-shell-client-protocol.h
SOURCES = main.c

all: release

wayland-play: $(HEADERS) $(SOURCES) $(OBJS) Makefile
	ctags --kinds-C=+l *.h *.c
	vtags.sed tags > .tags.vim
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(OBJS) $(WAYLAND_FLAGS) -lxkbcommon

$(OBJS): Makefile $(HEADERS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

xdg-shell-protocol.o: xdg-shell-protocol.c
	$(CC) -std=c99 -c -o $@ xdg-shell-protocol.c

xdg-shell-client-protocol.h:
	$(WAYLAND_SCANNER) client-header $(XDG_SHELL_PROTOCOL) xdg-shell-client-protocol.h

xdg-shell-protocol.c:
	$(WAYLAND_SCANNER) private-code $(XDG_SHELL_PROTOCOL) xdg-shell-protocol.c

clean:
	$(RM) wayland-play xdg-shell-protocol.c xdg-shell-client-protocol.h

install:
	install -Dm755 wayland-play ${DESTDIR}${PREFIX}/bin/wayland-play
uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/wayland-play
