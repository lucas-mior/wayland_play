#!/bin/sh
set -eu

PREFIX=${PREFIX:-/usr/local}
PKG_CONFIG=${PKG_CONFIG:-pkg-config}
RM=${RM:-rm -f}

PROG=wayland-play
OBJS=xdg-shell-protocol.o
HEADERS=xdg-shell-client-protocol.h
SOURCES=main.c

wayland_config_loaded=false

base_cflags() {
    printf '%s' "${CFLAGS:+$CFLAGS }"
    printf '%s' '-std=c11 -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=700 '
    printf '%s' '-Icbase -Wall -Wextra'
}

load_wayland_config() {
    if [ "$wayland_config_loaded" = true ]; then
        return
    fi

    WAYLAND_FLAGS=$($PKG_CONFIG wayland-client --cflags --libs)
    WAYLAND_PROTOCOLS_DIR=$($PKG_CONFIG wayland-protocols --variable=pkgdatadir)
    WAYLAND_SCANNER=$(pkg-config --variable=wayland_scanner wayland-scanner)
    XDG_SHELL_PROTOCOL=$WAYLAND_PROTOCOLS_DIR/stable/xdg-shell/xdg-shell.xml

    wayland_config_loaded=true
}

needs_rebuild() {
    target=$1
    shift

    if [ ! -e "$target" ]; then
        return 0
    fi

    for dep do
        if [ ! -e "$dep" ] || [ "$dep" -nt "$target" ]; then
            return 0
        fi
    done

    return 1
}

ensure_xdg_shell_header() {
    load_wayland_config

    if [ ! -f xdg-shell-client-protocol.h ]; then
        "$WAYLAND_SCANNER" client-header \
            "$XDG_SHELL_PROTOCOL" \
            xdg-shell-client-protocol.h
    fi
}

ensure_xdg_shell_source() {
    load_wayland_config

    if [ ! -f xdg-shell-protocol.c ]; then
        "$WAYLAND_SCANNER" private-code \
            "$XDG_SHELL_PROTOCOL" \
            xdg-shell-protocol.c
    fi
}

build_xdg_shell_object() {
    : "${BUILD_CC:=${CC:-cc}}"

    ensure_xdg_shell_header
    ensure_xdg_shell_source

    if needs_rebuild xdg-shell-protocol.o \
        xdg-shell-protocol.c \
        xdg-shell-client-protocol.h \
        "$0"; then
        $BUILD_CC -std=c99 -c -o xdg-shell-protocol.o xdg-shell-protocol.c
    fi
}

build_wayland_play() {
    : "${BUILD_CC:=${CC:-cc}}"
    : "${BUILD_CFLAGS:=$(base_cflags)}"

    ensure_xdg_shell_header
    build_xdg_shell_object
    load_wayland_config

    if needs_rebuild "$PROG" \
        $HEADERS \
        $SOURCES \
        $OBJS \
        cbase/*.c \
        cbase/*.h \
        "$0"; then
        ctags --kinds-C=+l *.h *.c
        vtags.sed tags > .tags.vim
        $BUILD_CC $BUILD_CFLAGS -o "$PROG" \
            $SOURCES $OBJS $WAYLAND_FLAGS -lxkbcommon
    fi
}

clean_target() {
    $RM wayland-play xdg-shell-protocol.c xdg-shell-client-protocol.h
}

release_target() {
    BUILD_CC=${CC:-cc}
    BUILD_CFLAGS="$(base_cflags) -O2 -flto"
    build_wayland_play
}

debug_target() {
    clean_target
    BUILD_CC=${CC:-cc}
    BUILD_CFLAGS="$(base_cflags) -g -DDEBUGGING=1 -fsanitize=undefined"
    build_wayland_play
}

clang_target() {
    clean_target
    BUILD_CC=clang
    BUILD_CFLAGS="$(base_cflags) -Weverything -Wno-unsafe-buffer-usage"
    build_wayland_play
}

install_target() {
    install -Dm755 wayland-play "${DESTDIR:-}$PREFIX/bin/wayland-play"
}

uninstall_target() {
    rm -f "${DESTDIR:-}$PREFIX/bin/wayland-play"
}

run_target() {
    case $1 in
        all)
            release_target
            ;;
        release)
            release_target
            ;;
        debug)
            debug_target
            ;;
        clang)
            clang_target
            ;;
        wayland-play)
            BUILD_CC=${CC:-cc}
            BUILD_CFLAGS=$(base_cflags)
            build_wayland_play
            ;;
        xdg-shell-client-protocol.h)
            ensure_xdg_shell_header
            ;;
        xdg-shell-protocol.c)
            ensure_xdg_shell_source
            ;;
        xdg-shell-protocol.o)
            BUILD_CC=${CC:-cc}
            build_xdg_shell_object
            ;;
        clean)
            clean_target
            ;;
        install)
            install_target
            ;;
        uninstall)
            uninstall_target
            ;;
        *)
            cat >&2 <<EOF
usage: $0 [all|release|debug|clang|wayland-play|\
xdg-shell-client-protocol.h|xdg-shell-protocol.c|xdg-shell-protocol.o|\
clean|install|uninstall]
EOF
            exit 2
            ;;
    esac
}

if [ $# -eq 0 ]; then
    set -- all
fi

for target do
    run_target "$target"
done
