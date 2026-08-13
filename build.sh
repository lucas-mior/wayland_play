#!/bin/sh -e

# shellcheck disable=SC2086

dir=$(dirname "$(readlink -f "$0")")
cd "$dir" || exit

# shellcheck source=./cbase/common.sh
. "./cbase/common.sh"

program=$(common_get_program "$0")
script=$(basename "$0")


common_build_parse_args "$@"

common_build_print_invocation "$script"

PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

exe="bin/$program"
xdg_header="xdg-shell-client-protocol.h"
xdg_source="xdg-shell-protocol.c"
xdg_object="bin/xdg-shell-protocol.o"
mkdir -p "$(dirname "$exe")"

CC=$(common_get_compiler "$mode")

CPPFLAGS="$CPPFLAGS -Icbase"

CFLAGS="$CFLAGS -std=c11"
CFLAGS="$CFLAGS -Wfatal-errors"
CFLAGS="$CFLAGS -Wextra -Wall"
CFLAGS="$CFLAGS -Werror=all -Werror=extra"
CFLAGS="$CFLAGS -Werror"  # Only uncomment occasionally, keep this line

if [ "$CC" = "clang" ]; then
    CFLAGS="$CFLAGS -Weverything"
    CFLAGS="$CFLAGS -Wno-assign-enum"
    CFLAGS="$CFLAGS -Wno-c++-keyword"
    CFLAGS="$CFLAGS -Wno-cast-qual"
    CFLAGS="$CFLAGS -Wno-char-subscripts"
    CFLAGS="$CFLAGS -Wno-constant-logical-operand"
    CFLAGS="$CFLAGS -Wno-covered-switch-default"
    CFLAGS="$CFLAGS -Wno-disabled-macro-expansion"
    CFLAGS="$CFLAGS -Wno-float-equal"
    CFLAGS="$CFLAGS -Wno-format-nonliteral"
    CFLAGS="$CFLAGS -Wno-implicit-int-enum-cast"
    CFLAGS="$CFLAGS -Wno-implicit-void-ptr-cast"
    CFLAGS="$CFLAGS -Wno-padded"
    CFLAGS="$CFLAGS -Wno-pre-c11-compat"
    CFLAGS="$CFLAGS -Wno-unsafe-buffer-usage"
    CFLAGS="$CFLAGS -Wno-unused-macros"
    CFLAGS="$CFLAGS -Wno-used-but-marked-unused"
fi

case "$mode" in
debug)
    CFLAGS="$CFLAGS -g3 -Og"
    CPPFLAGS="$CPPFLAGS -DDEBUGGING=1 -Wno-unused-function"
    exe="bin/${program}_debug"
    ;;
build)
    CFLAGS="$CFLAGS -O2 -flto -march=native -ftree-vectorize"
    ;;
fast_feedback)
    ;;
test|install|uninstall)
    ;;
*)
    ;;
esac

case "$OS" in
*Linux*)
    ;;
*Darwin*)
    CPPFLAGS="$CPPFLAGS -D_DARWIN_C_SOURCE"
    ;;
esac


case "$mode" in
test)
    TEST_EXCLUDE_PATTERN='(^|/)cbase/' common_test "$target"
    exit
    ;;
uninstall)
    trace_on
    rm -f "${DESTDIR}${PREFIX}/bin/${program}"
    trace_off
    ;;
install)
    if [ ! -f "bin/$program" ]; then
        "$0" build
    fi

    trace_on
    install -Dm755 "bin/$program" "${DESTDIR}${PREFIX}/bin/${program}"
    trace_off
    ;;
build|debug|fast_feedback)
    WAYLAND_CFLAGS=$($PKG_CONFIG wayland-client xkbcommon --cflags)
    WAYLAND_LDFLAGS=$($PKG_CONFIG wayland-client xkbcommon --libs)
    WAYLAND_PROTOCOLS_DIR=$($PKG_CONFIG wayland-protocols --variable=pkgdatadir)
    WAYLAND_SCANNER=$($PKG_CONFIG wayland-scanner --variable=wayland_scanner)
    XDG_SHELL_PROTOCOL=$WAYLAND_PROTOCOLS_DIR/stable/xdg-shell/xdg-shell.xml

    CPPFLAGS="$CPPFLAGS $WAYLAND_CFLAGS"
    LDFLAGS="$LDFLAGS $WAYLAND_LDFLAGS"

    if common_outdated "$xdg_header" "$XDG_SHELL_PROTOCOL" "$0"; then
        "$WAYLAND_SCANNER" client-header \
            "$XDG_SHELL_PROTOCOL" \
            "$xdg_header"
    fi

    if common_outdated "$xdg_source" "$XDG_SHELL_PROTOCOL" "$0"; then
        "$WAYLAND_SCANNER" private-code \
            "$XDG_SHELL_PROTOCOL" \
            "$xdg_source"
    fi

    if common_outdated "$xdg_object" "$xdg_source" "$xdg_header" "$0"; then
        $CC $CPPFLAGS $WAYLAND_CFLAGS -std=c99 -c -o "$xdg_object" "$xdg_source"
    fi

    common_build_tags

    trace_on
    $CC $CPPFLAGS $CFLAGS -o "$exe" main.c "$xdg_object" $LDFLAGS
    trace_off
    ;;
esac


case "$mode" in
build|debug|fast_feedback|install|test|uninstall)
    ;;
*)
    echo "Unknown mode $mode"
    exit 1
    ;;
esac
