/*
 * Copyright (C) 2024 Lucas Mior

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define CBASE_IMPLEMENT
#include "wayland-play.h"

#define NORMAL_EXIT_MESSAGE \
    "\n========== wayland-play: normal exit ==========\n"

static void resize_window(int32 width, int32 height);

static bool running = true;

static Window window;
static Keyboard keyboard;

static uint32 palette_red[PALETTE_COLORS] = {
    0xFF0000,
    0xCC0000,
    0x770000,
    0x440000,
};

static uint32 palette_green[PALETTE_COLORS] = {
    0x00FF00,
    0x00CC00,
    0x007700,
    0x004400,
};

static uint32 palette_blue[PALETTE_COLORS] = {
    0x0000FF,
    0x0000CC,
    0x000077,
    0x000044,
};

static uint32 palette_gray[PALETTE_COLORS] = {
    0x000000,
    0xCCCCCC,
    0x777777,
    0x444444,
};

static uint32 *palettes[] = {
    palette_red,
    palette_green,
    palette_blue,
    palette_gray,
};

static void
window_set_palette(uint32 *palette) {
    memcpy64(window.palette, palette, SIZEOF(window.palette));
    return;
}

static void
xdg_surface_configure(
    void *data,
    struct xdg_surface *xdg_surface,
    uint32 serial
) {
    (void)data;

    if (window.dirty) {
        wl_surface_attach(window.surface, window.buffer, 0, 0);
        wl_surface_damage(
            window.surface,
            0,
            0,
            WINDOW_WIDTH_MAX,
            WINDOW_HEIGHT_MAX
        );
        window.dirty = false;
    }

    xdg_surface_ack_configure(xdg_surface, serial);
    wl_surface_commit(window.surface);
    return;
}

static struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void
xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel) {
    (void)data;
    (void)toplevel;

    running = false;
    return;
}

static void
xdg_toplevel_configure(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    int32 width,
    int32 height,
    struct wl_array *states
) {
    (void)data;
    (void)xdg_toplevel;
    (void)states;

    resize_window(width, height);
    return;
}

static void
xdg_toplevel_configure_bounds(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    int32 width,
    int32 height
) {
    (void)data;
    (void)xdg_toplevel;
    (void)width;
    (void)height;

    return;
}

static void
xdg_toplevel_wm_capabilities(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    struct wl_array *capabilities
) {
    (void)data;
    (void)xdg_toplevel;
    (void)capabilities;

    return;
}

static struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
    .configure_bounds = xdg_toplevel_configure_bounds,
    .wm_capabilities = xdg_toplevel_wm_capabilities,
};

static void
wl_pointer_enter(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32 serial,
    struct wl_surface *surface,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y
) {
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)surface;
    (void)surface_x;
    (void)surface_y;

    return;
}

static void
wl_pointer_leave(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32 serial,
    struct wl_surface *surface
) {
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)surface;

    return;
}

static void
set_window_colors(uint32 *buffer, int32 x, int32 y) {
    uint32 alpha;

    alpha = window.alpha << ALPHA_SHIFT;
    for (int32 j = 0; j < window.height; j += 1) {
        for (int32 i = 0; i < window.width; i += 1) {
            if (i < x) {
                if (j < y) {
                    buffer[j*window.width + i] = alpha | window.palette[0];
                } else {
                    buffer[j*window.width + i] = alpha | window.palette[1];
                }
            } else {
                if (j < y) {
                    buffer[j*window.width + i] = alpha | window.palette[2];
                } else {
                    buffer[j*window.width + i] = alpha | window.palette[3];
                }
            }
        }
    }

    wl_surface_attach(window.surface, window.buffer, 0, 0);
    wl_surface_damage(window.surface, 0, 0, window.width, window.height);
    wl_surface_commit(window.surface);
    window.dirty = false;
    return;
}

static void
wl_pointer_motion(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32 time,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y
) {
    (void)data;
    (void)wl_pointer;
    (void)time;

    window.x = wl_fixed_to_int(surface_x);
    window.y = wl_fixed_to_int(surface_y);

    set_window_colors(window.draw_buffer, window.x, window.y);
    return;
}

static void
wl_pointer_button(
    void *data,
    struct wl_pointer *pointer,
    uint32 serial,
    uint32 time,
    uint32 button,
    uint32 state
) {
    struct wl_seat *seat = data;

    (void)pointer;
    (void)time;

    if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
        switch (button) {
        case BTN_LEFT:
            xdg_toplevel_move(window.xdg_toplevel, seat, serial);
            break;
        case BTN_RIGHT: {
            uint32 resize_type = XDG_TOPLEVEL_RESIZE_EDGE_NONE;

            if (window.y < window.height/2) {
                resize_type |= XDG_TOPLEVEL_RESIZE_EDGE_TOP;
            } else {
                resize_type |= XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
            }
            if (window.x < window.width/2) {
                resize_type |= XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
            } else {
                resize_type |= XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
            }

            xdg_toplevel_resize(
                window.xdg_toplevel,
                seat,
                serial,
                resize_type
            );
            break;
        }
        case BTN_MIDDLE:
            xdg_toplevel_configure(
                NULL,
                window.xdg_toplevel,
                WINDOW_WIDTH,
                WINDOW_HEIGHT,
                NULL
            );
            set_window_colors(window.draw_buffer, window.x, window.y);
            break;
        default:
            break;
        }
    }

    return;
}

static void
wl_pointer_axis(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32 time,
    uint32 axis,
    wl_fixed_t value
) {
    (void)data;
    (void)wl_pointer;
    (void)time;
    (void)axis;
    (void)value;

    return;
}

static void
wl_pointer_frame(void *data, struct wl_pointer *wl_pointer) {
    (void)data;
    (void)wl_pointer;

    return;
}

static void
wl_pointer_axis_source(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32 axis_source
) {
    (void)data;
    (void)wl_pointer;
    (void)axis_source;

    return;
}

static void
wl_pointer_axis_stop(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32 time,
    uint32 axis
) {
    (void)data;
    (void)wl_pointer;
    (void)time;
    (void)axis;

    return;
}

static void
wl_pointer_axis_discrete(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32 axis,
    int32 discrete
) {
    (void)data;
    (void)wl_pointer;
    (void)axis;
    (void)discrete;

    return;
}

static void
wl_pointer_axis_value120(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32 axis,
    int32 value120
) {
    (void)data;
    (void)wl_pointer;
    (void)axis;
    (void)value120;

    return;
}

static void
wl_pointer_axis_relative_direction(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32 axis,
    uint32 direction
) {
    (void)data;
    (void)wl_pointer;
    (void)axis;
    (void)direction;

    return;
}

static struct wl_pointer_listener wl_pointer_listener = {
    .enter = wl_pointer_enter,
    .leave = wl_pointer_leave,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
    .axis = wl_pointer_axis,
    .frame = wl_pointer_frame,
    .axis_source = wl_pointer_axis_source,
    .axis_stop = wl_pointer_axis_stop,
    .axis_discrete = wl_pointer_axis_discrete,
    .axis_value120 = wl_pointer_axis_value120,
    .axis_relative_direction = wl_pointer_axis_relative_direction,
};

static void
wl_keyboard_keymap(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32 format,
    int32 fd,
    uint32 size
) {
    char *keymap_name;

    (void)data;
    (void)wl_keyboard;
    (void)format;

    keymap_name = mmap(NULL, (size_t)size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (keymap_name == MAP_FAILED) {
        error("Error mapping keyboard data: %s.\n", strerror(errno));
        XCLOSE(&fd);
        return;
    }

    xkb_state_unref(keyboard.xkb_state);
    xkb_keymap_unref(keyboard.xkb_keymap);
    xkb_context_unref(keyboard.xkb_context);

    keyboard.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (keyboard.xkb_context == NULL) {
        error("Error creating XKB context.\n");
        fatal(EXIT_FAILURE);
    }

    keyboard.xkb_keymap = xkb_keymap_new_from_string(
        keyboard.xkb_context,
        keymap_name,
        XKB_KEYMAP_FORMAT_TEXT_V1,
        XKB_KEYMAP_COMPILE_NO_FLAGS
    );
    if (keyboard.xkb_keymap == NULL) {
        error("Error creating XKB keymap.\n");
        fatal(EXIT_FAILURE);
    }

    keyboard.xkb_state = xkb_state_new(keyboard.xkb_keymap);
    if (keyboard.xkb_state == NULL) {
        error("Error creating XKB state.\n");
        fatal(EXIT_FAILURE);
    }

    munmap(keymap_name, (size_t)size);
    XCLOSE(&fd);
    return;
}

static void
wl_keyboard_enter(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32 serial,
    struct wl_surface *surface,
    struct wl_array *keys
) {
    (void)data;
    (void)wl_keyboard;
    (void)serial;
    (void)surface;
    (void)keys;

    return;
}

static void
wl_keyboard_leave(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32 serial,
    struct wl_surface *surface
) {
    (void)data;
    (void)wl_keyboard;
    (void)serial;
    (void)surface;

    return;
}

static void
wl_keyboard_key(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32 serial,
    uint32 time,
    uint32 key,
    uint32 state
) {
    uint32 keycode;
    uint32 keysym;
    uint32 utf32;

    (void)data;
    (void)wl_keyboard;
    (void)serial;
    (void)time;

    keycode = key + WAYLAND_KEYCODE_OFFSET;
    keysym = xkb_state_key_get_one_sym(keyboard.xkb_state, keycode);
    utf32 = xkb_keysym_to_utf32(keysym);

    if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        switch (utf32) {
        case XKB_KEY_q:
            running = false;
            break;
        case XKB_KEY_u:
            window_set_palette(palette_red);
            break;
        case XKB_KEY_i:
            window_set_palette(palette_green);
            break;
        case XKB_KEY_o:
            window_set_palette(palette_blue);
            break;
        case XKB_KEY_p:
            window_set_palette(palette_gray);
            break;
        case XKB_KEY_r:
            window_set_palette(palettes[rand() % LENGTH(palettes)]);
            break;
        case XKB_KEY_k:
            if (window.alpha >= ALPHA_STEP) {
                window.alpha -= ALPHA_STEP;
            }
            break;
        case XKB_KEY_l:
            if (window.alpha <= ALPHA_MAX - ALPHA_STEP) {
                window.alpha += ALPHA_STEP;
            }
            break;
        default:
            break;
        }
        set_window_colors(window.draw_buffer, window.x, window.y);
    }

    return;
}

static void
wl_keyboard_modifiers(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32 serial,
    uint32 mods_depressed,
    uint32 mods_latched,
    uint32 mods_locked,
    uint32 group
) {
    (void)data;
    (void)wl_keyboard;
    (void)serial;
    (void)mods_depressed;
    (void)mods_latched;
    (void)mods_locked;
    (void)group;

    return;
}

static void
wl_keyboard_repeat_info(
    void *data,
    struct wl_keyboard *wl_keyboard,
    int32 rate,
    int32 delay
) {
    (void)data;
    (void)wl_keyboard;
    (void)rate;
    (void)delay;

    return;
}

static struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .key = wl_keyboard_key,
    .modifiers = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};

static void
seat_capabilities(
    void *data,
    struct wl_seat *seat,
    uint32 capabilities
) {
    (void)data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        window.wl_pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(window.wl_pointer, &wl_pointer_listener, seat);
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        window.wl_keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(
            window.wl_keyboard,
            &wl_keyboard_listener,
            seat
        );
    }

    return;
}

static void
seat_name(
    void *data,
    struct wl_seat *wl_seat,
    const char *name
) {
    (void)data;
    (void)wl_seat;
    (void)name;

    return;
}

static struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};

static void
xdg_wm_base_listener_ping(
    void *data,
    struct xdg_wm_base *wm_base,
    uint32 serial
) {
    (void)data;

    xdg_wm_base_pong(wm_base, serial);
    return;
}

static struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_listener_ping,
};

static bool
interface_matches(char *interface, char *name) {
    bool result;

    result = strequal(interface, name);
    return result;
}

static void
wl_registry_global(
    void *data,
    struct wl_registry *registry,
    uint32 name,
    const char *interface,
    uint32 version
) {
    (void)data;
    (void)version;

    if (interface_matches((char *)interface, (char *)wl_shm_interface.name)) {
        window.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (interface_matches(
        (char *)interface,
        (char *)wl_seat_interface.name
    )) {
        struct wl_seat *seat;

        seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    } else if (interface_matches(
        (char *)interface,
        (char *)wl_compositor_interface.name
    )) {
        window.compositor = wl_registry_bind(
            registry,
            name,
            &wl_compositor_interface,
            1
        );
    } else if (interface_matches(
        (char *)interface,
        (char *)xdg_wm_base_interface.name
    )) {
        window.xdg_wm_base = wl_registry_bind(
            registry,
            name,
            &xdg_wm_base_interface,
            1
        );
    }

    return;
}

static void
wl_registry_global_remove(
    void *data,
    struct wl_registry *registry,
    uint32 name
) {
    (void)data;
    (void)registry;
    (void)name;

    return;
}

static struct wl_registry_listener wl_registry_listener = {
    .global = wl_registry_global,
    .global_remove = wl_registry_global_remove,
};

static void
resize_window(int32 width, int32 height) {
    int32 stride;

    if ((width <= 0) || (height <= 0)) {
        return;
    }

    window.width = (int32)CLAMP(
        width,
        WINDOW_WIDTH_MIN,
        WINDOW_WIDTH_MAX
    );
    window.height = (int32)CLAMP(
        height,
        WINDOW_HEIGHT_MIN,
        WINDOW_HEIGHT_MAX
    );

    stride = window.width*BYTES_PER_PIXEL;
    if (window.buffer) {
        wl_buffer_destroy(window.buffer);
    }
    window.buffer = wl_shm_pool_create_buffer(
        window.shm_pool,
        0,
        window.width,
        window.height,
        stride,
        WL_SHM_FORMAT_ARGB8888
    );
    window.dirty = true;
    return;
}

static void
create_buffer(void) {
    int32 fd;
    int32 retries;

    fd = -1;
    retries = SHM_CREATE_RETRIES;
    window.alloc_size = BYTES_PER_PIXEL*WINDOW_WIDTH_MAX*WINDOW_HEIGHT_MAX;

    do {
        char name[] = "/wayland-play-XXXXXX";
        char *random_part;
        struct timespec ts;
        int64 r;

        random_part = name + SIZEOF(name) - SHM_NAME_RANDOM_BYTES - 1;
        if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
            error("Error reading time: %s.\n", strerror(errno));
            fatal(EXIT_FAILURE);
        }

        r = ts.tv_nsec;
        for (int32 i = 0; i < SHM_NAME_RANDOM_BYTES; i += 1) {
            random_part[i] = (char)('A' + (r & 15) + (r & 16)*2);
            r >>= 5;
        }

        retries -= 1;
        fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            break;
        }
        if (errno != EEXIST) {
            error("Error opening shared memory object: %s.\n", strerror(errno));
            fatal(EXIT_FAILURE);
        }
    } while (retries > 0);

    if (fd < 0) {
        error("Error creating shared memory file: %s.\n", strerror(errno));
        fatal(EXIT_FAILURE);
    }

    if (ftruncate(fd, window.alloc_size) < 0) {
        error("Error truncating shared memory file: %s.\n", strerror(errno));
        XCLOSE(&fd);
        fatal(EXIT_FAILURE);
    }

    window.draw_buffer = mmap(
        NULL,
        (size_t)window.alloc_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );
    if (window.draw_buffer == MAP_FAILED) {
        error("mmap failed: %s.\n", strerror(errno));
        XCLOSE(&fd);
        fatal(EXIT_FAILURE);
    }

    window.shm_pool = wl_shm_create_pool(window.shm, fd, window.alloc_size);
    XCLOSE(&fd);

    resize_window(WINDOW_WIDTH, WINDOW_HEIGHT);
    memset64(window.draw_buffer, 0xFF, window.alloc_size);
    return;
}

int32
main(int32 argc, char **argv) {
    (void)argc;
    (void)argv;

    window.alpha = ALPHA_DEFAULT;
    window_set_palette(palette_red);

    if ((window.display = wl_display_connect(NULL)) == NULL) {
        error("Error connecting to display: %s.\n", strerror(errno));
        fatal(EXIT_FAILURE);
    }

    window.registry = wl_display_get_registry(window.display);
    wl_registry_add_listener(window.registry, &wl_registry_listener, NULL);
    wl_display_roundtrip(window.display);

    if ((window.shm == NULL)
        || (window.compositor == NULL)
        || (window.xdg_wm_base == NULL)) {
        error(
            "Error: missing wl_shm, wl_compositor, or xdg_wm_base support.\n"
        );
        fatal(EXIT_FAILURE);
    }

    create_buffer();

    window.surface = wl_compositor_create_surface(window.compositor);
    window.xdg_surface = xdg_wm_base_get_xdg_surface(
        window.xdg_wm_base,
        window.surface
    );
    window.xdg_toplevel = xdg_surface_get_toplevel(window.xdg_surface);

    xdg_toplevel_set_min_size(
        window.xdg_toplevel,
        WINDOW_WIDTH_MIN,
        WINDOW_HEIGHT_MIN
    );
    xdg_toplevel_set_max_size(
        window.xdg_toplevel,
        WINDOW_WIDTH_MAX,
        WINDOW_HEIGHT_MAX
    );
    xdg_toplevel_set_title(window.xdg_toplevel, "wayland-play");

    xdg_surface_add_listener(
        window.xdg_surface,
        &xdg_surface_listener,
        NULL
    );
    xdg_toplevel_add_listener(
        window.xdg_toplevel,
        &xdg_toplevel_listener,
        NULL
    );
    xdg_wm_base_add_listener(
        window.xdg_wm_base,
        &xdg_wm_base_listener,
        NULL
    );

    wl_surface_commit(window.surface);
    wl_display_roundtrip(window.display);

    wl_surface_attach(window.surface, window.buffer, 0, 0);
    wl_surface_commit(window.surface);

    while (running) {
        if (wl_display_dispatch(window.display) == -1) {
            break;
        }
    }

    xkb_state_unref(keyboard.xkb_state);
    xkb_keymap_unref(keyboard.xkb_keymap);
    xkb_context_unref(keyboard.xkb_context);
    xdg_toplevel_destroy(window.xdg_toplevel);
    xdg_surface_destroy(window.xdg_surface);
    wl_surface_destroy(window.surface);
    wl_buffer_destroy(window.buffer);

    write_all(STDERR_FILENO, STRLIT(NORMAL_EXIT_MESSAGE));
    write_all(STDOUT_FILENO, STRLIT(NORMAL_EXIT_MESSAGE));
    exit(EXIT_SUCCESS);
}

#undef NORMAL_EXIT_MESSAGE
