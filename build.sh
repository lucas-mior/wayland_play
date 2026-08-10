#!/bin/sh -e

# shellcheck disable=SC2086

dir=$(dirname "$(readlink -f "$0")")
# shellcheck source=/dev/null
. "$dir/cbase/common.sh"

cd "$dir" || exit
program=$(get_program "$0")
script=$(basename "$0")

if [ -f ./targets ]; then
    targets=$(cat ./targets)
else
    targets=$(cat <<'EOF_TARGETS'
build
debug
fast_feedback
install
uninstall
test
EOF_TARGETS
)
fi

target="${1:-debug}"

if ! printf '%s\n' "$targets" | grep -qx "$target"; then
    echo "usage: $script <targets>"
    printf '%s\n' "$targets"
    exit 1
fi

printf "\n${script} ${RED}${1:-} ${2:-}$RES\n"

PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

exe="bin/$program"
xdg_header="xdg-shell-client-protocol.h"
xdg_source="xdg-shell-protocol.c"
xdg_object="bin/xdg-shell-protocol.o"
mkdir -p "$(dirname "$exe")"

CC=$(get_compiler "$target")

CPPFLAGS="$CPPFLAGS -I$dir/cbase"
CPPFLAGS="$CPPFLAGS -D_DEFAULT_SOURCE"

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

OS=$(uname -a)

GNUSOURCE=

if echo "$OS" | grep -q "Linux"; then
    if echo "$OS" | grep -q "GNU"; then
        GNUSOURCE="-D_GNU_SOURCE"
    fi
fi

case "$target" in
debug)
    CFLAGS="$CFLAGS -g3 -O0 -fsanitize=undefined"
    CPPFLAGS="$CPPFLAGS $GNUSOURCE -DDEBUGGING=1 -Wno-unused-function"
    exe="bin/${program}_debug"
    ;;
build)
    CFLAGS="$CFLAGS $GNUSOURCE -O2 -flto -march=native -ftree-vectorize"
    ;;
fast_feedback)
    CFLAGS="$CFLAGS $GNUSOURCE"
    ;;
test|install|uninstall)
    ;;
*)
    CFLAGS="$CFLAGS -O2"
    ;;
esac

case "$OS" in
*Linux*)
    CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=700"
    ;;
*Darwin*)
    CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=700 -D_DARWIN_C_SOURCE"
    ;;
esac


build_program () {
    WAYLAND_CFLAGS=$($PKG_CONFIG wayland-client xkbcommon --cflags)
    WAYLAND_LDFLAGS=$($PKG_CONFIG wayland-client xkbcommon --libs)
    WAYLAND_PROTOCOLS_DIR=$($PKG_CONFIG wayland-protocols --variable=pkgdatadir)
    WAYLAND_SCANNER=$($PKG_CONFIG wayland-scanner --variable=wayland_scanner)
    XDG_SHELL_PROTOCOL=$WAYLAND_PROTOCOLS_DIR/stable/xdg-shell/xdg-shell.xml

    CPPFLAGS="$CPPFLAGS $WAYLAND_CFLAGS"
    LDFLAGS="$LDFLAGS $WAYLAND_LDFLAGS"

    if needs_rebuild "$xdg_header" "$XDG_SHELL_PROTOCOL" "$0"; then
        "$WAYLAND_SCANNER" client-header \
            "$XDG_SHELL_PROTOCOL" \
            "$xdg_header"
    fi

    if needs_rebuild "$xdg_source" "$XDG_SHELL_PROTOCOL" "$0"; then
        "$WAYLAND_SCANNER" private-code \
            "$XDG_SHELL_PROTOCOL" \
            "$xdg_source"
    fi

    if needs_rebuild "$xdg_object" "$xdg_source" "$xdg_header" "$0"; then
        $CC $WAYLAND_CFLAGS -std=c99 -c -o "$xdg_object" "$xdg_source"
    fi

    build_tags

    trace_on
    $CC $CPPFLAGS $CFLAGS -o "$exe" main.c "$xdg_object" $LDFLAGS
    trace_off
}

case "$target" in
fast_feedback)
    build_program
    LC_ALL=C "$exe"
    ;;
test)
    TEST_EXCLUDE_PATTERN='(^|/)cbase/' test "$2"
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
*)
    build_program
    ;;
esac
