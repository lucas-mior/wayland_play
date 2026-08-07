#!/bin/sh -e

set -e

if [ -n "$BASH_VERSION" ]; then
    # shellcheck disable=SC3044
    shopt -s expand_aliases
fi

alias trace_on='set -x'
alias trace_off='{ set +x; } 2>/dev/null'

dir=$(dirname "$(readlink -f "$0")")
CPPFLAGS="$CPPFLAGS -I$dir/cbase"
cd "$dir" || exit
program=$(basename "$(readlink -f "$(dirname "$0")")")
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

printf "\n%s %s\n" "$script" "$target"

PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

main="main.c"
exe="bin/$program"
xdg_header="xdg-shell-client-protocol.h"
xdg_source="xdg-shell-protocol.c"
xdg_object="bin/xdg-shell-protocol.o"
mkdir -p "$(dirname "$exe")"

CPPFLAGS="$CPPFLAGS -D_DEFAULT_SOURCE"
CFLAGS="$CFLAGS -std=c11"
CFLAGS="$CFLAGS -Wfatal-errors"
CFLAGS="$CFLAGS -Wextra -Wall"
CFLAGS="$CFLAGS -Werror"
CFLAGS="$CFLAGS -Wno-format-pedantic"
CFLAGS="$CFLAGS -Wno-unknown-warning-option"
CFLAGS="$CFLAGS -Wno-gnu-union-cast"
CFLAGS="$CFLAGS -Wno-unused-macros"
CFLAGS="$CFLAGS -Wno-constant-logical-operand"
CFLAGS="$CFLAGS -Wno-float-equal"
CFLAGS="$CFLAGS -Wno-undefined-internal"
CFLAGS="$CFLAGS -Wno-cast-qual"
CFLAGS="$CFLAGS -Wno-unknown-pragmas"
CFLAGS="$CFLAGS -Wno-char-subscripts"

OS=$(uname -a)
requested_cc=${CC:-}
case "$target" in
"debug"|"test"|"fast_feedback")
    CC="${requested_cc:-tcc}"
    ;;
*)
    CC="${requested_cc:-cc}"
    ;;
esac
GNUSOURCE=

if [ "$CC" = "clang" ]; then
    CFLAGS="$CFLAGS -Weverything"
    CFLAGS="$CFLAGS -Wno-unsafe-buffer-usage"
    CFLAGS="$CFLAGS -Wno-format-nonliteral"
    CFLAGS="$CFLAGS -Wno-disabled-macro-expansion"
    CFLAGS="$CFLAGS -Wno-c++-keyword"
    CFLAGS="$CFLAGS -Wno-pre-c11-compat"
    CFLAGS="$CFLAGS -Wno-implicit-void-ptr-cast"
    CFLAGS="$CFLAGS -Wno-ignored-attributes"
    CFLAGS="$CFLAGS -Wno-covered-switch-default"
    CFLAGS="$CFLAGS -Wno-used-but-marked-unused"
    CFLAGS="$CFLAGS -Wno-implicit-int-enum-cast"
    CFLAGS="$CFLAGS -Wno-assign-enum"
    CFLAGS="$CFLAGS -Wno-cast-function-type-strict"
    CFLAGS="$CFLAGS -Wno-bad-function-cast"
    CFLAGS="$CFLAGS -Wno-char-subscripts"
    CFLAGS="$CFLAGS -Wno-padded"
fi

if echo "$OS" | grep -q "Linux"; then
    if echo "$OS" | grep -q "GNU"; then
        GNUSOURCE="-D_GNU_SOURCE"
    fi
fi

case "$target" in
"debug")
    CFLAGS="$CFLAGS -g3 -O0 -fsanitize=undefined"
    CPPFLAGS="$CPPFLAGS $GNUSOURCE -DDEBUGGING=1 -Wno-unused-function"
    exe="bin/${program}_debug"
    ;;
"build")
    CFLAGS="$CFLAGS $GNUSOURCE -g3 -O2 -flto -march=native -ftree-vectorize"
    ;;
"fast_feedback")
    CFLAGS="$CFLAGS $GNUSOURCE -Werror"
    ;;
"test"|"install"|"uninstall")
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

wayland_config_loaded=false

load_wayland_config () {
    if [ "$wayland_config_loaded" = true ]; then
        return
    fi

    WAYLAND_CFLAGS=$($PKG_CONFIG wayland-client xkbcommon --cflags)
    WAYLAND_LDFLAGS=$($PKG_CONFIG wayland-client xkbcommon --libs)
    WAYLAND_PROTOCOLS_DIR=$($PKG_CONFIG wayland-protocols --variable=pkgdatadir)
    WAYLAND_SCANNER=$($PKG_CONFIG wayland-scanner --variable=wayland_scanner)
    XDG_SHELL_PROTOCOL=$WAYLAND_PROTOCOLS_DIR/stable/xdg-shell/xdg-shell.xml

    CPPFLAGS="$CPPFLAGS $WAYLAND_CFLAGS"
    LDFLAGS="$LDFLAGS $WAYLAND_LDFLAGS"
    wayland_config_loaded=true
}

needs_rebuild () {
    rebuild_target="$1"
    shift

    if [ ! -e "$rebuild_target" ]; then
        return 0
    fi

    for dep do
        if [ ! -e "$dep" ] || [ "$dep" -nt "$rebuild_target" ]; then
            return 0
        fi
    done

    return 1
}

ensure_xdg_shell_header () {
    load_wayland_config

    if needs_rebuild "$xdg_header" "$XDG_SHELL_PROTOCOL" "$0"; then
        "$WAYLAND_SCANNER" client-header \
            "$XDG_SHELL_PROTOCOL" \
            "$xdg_header"
    fi
}

ensure_xdg_shell_source () {
    load_wayland_config

    if needs_rebuild "$xdg_source" "$XDG_SHELL_PROTOCOL" "$0"; then
        "$WAYLAND_SCANNER" private-code \
            "$XDG_SHELL_PROTOCOL" \
            "$xdg_source"
    fi
}

build_xdg_shell_object () {
    ensure_xdg_shell_header
    ensure_xdg_shell_source

    if needs_rebuild "$xdg_object" "$xdg_source" "$xdg_header" "$0"; then
        $CC $WAYLAND_CFLAGS -std=c99 -c -o "$xdg_object" "$xdg_source"
    fi
}

build_tags () {
    if command -v ctags >/dev/null 2>&1; then
        find . -iname "*.[ch]" -print0 \
            | xargs -0 ctags --kinds-C=+l+d || true
    fi

    if [ -f tags ] && command -v vtags.sed >/dev/null 2>&1; then
        vtags.sed tags | sort | uniq > .tags.vim || true
    fi
}

build_program () {
    load_wayland_config
    build_xdg_shell_object
    build_tags

    trace_on
    $CC $CPPFLAGS $CFLAGS -o "$exe" "$main" "$xdg_object" $LDFLAGS
    trace_off
}

case "$target" in
"fast_feedback")
    build_program
    LC_ALL=C "$exe"
    ;;
"test")
    exit
    ;;
"uninstall")
    trace_on
    rm -f "${DESTDIR}${PREFIX}/bin/${program}"
    trace_off
    ;;
"install")
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
