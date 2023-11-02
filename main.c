#include "wayland-play.h"

static void resize_window(int width, int height);

static bool running = true;

static struct Window w;
static struct Keyboard k;

uint32 red[4] = { 0xFF0000, 0xCC0000, 0x770000, 0x440000 };
uint32 green[4] = { 0x00FF00, 0x00CC00, 0x007700, 0x004400 };
uint32 blue[4] = { 0x0000FF, 0x0000CC, 0x000077, 0x000044 };
uint32 gray[4] = { 0x000000, 0xCCCCCC, 0x777777, 0x444444 };
uint32 *palletes[4] = {
    red,
    green,
    blue,
    gray,
};

static void
xdg_surface_configure(
    void *data,
    struct xdg_surface *xdg_surface,
    uint32_t serial
) {
    (void) data;

    if (w.dirty) {
        wl_surface_attach(w.surface, w.buffer, 0, 0);
        wl_surface_damage(w.surface, 0, 0, WIDTH_MAX, HEIGHT_MAX);
        w.dirty = false;
    }
    xdg_surface_ack_configure(xdg_surface, serial);
    wl_surface_commit(w.surface);
}

static struct xdg_surface_listener
xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void
xdg_toplevel_close(
    void *data,
    struct xdg_toplevel *toplevel
) {
    (void) data;
    (void) toplevel;

    running = false;
    return;
}

static void
xdg_toplevel_configure(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array *states
) {
    (void) data;
    (void) xdg_toplevel;
    (void) states;

    resize_window(width, height);
    return;
}

static void
xdg_toplevel_configure_bounds(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    int32_t width,
    int32_t height
) {
    (void) data;
    (void) xdg_toplevel;
    (void) width;
    (void) height;

    return;
}

static void
xdg_toplevel_wm_capabilities(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    struct wl_array *capabilities
) {
    (void) data;
    (void) xdg_toplevel;
    (void) capabilities;

    return;
}

static struct xdg_toplevel_listener
xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
    .configure_bounds = xdg_toplevel_configure_bounds,
    .wm_capabilities = xdg_toplevel_wm_capabilities,
};

static void
wl_pointer_enter(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    struct wl_surface *surface,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y
) {
    (void) data;
    (void) wl_pointer;
    (void) serial;
    (void) surface;
    (void) surface_x;
    (void) surface_y;

    return;
}

static void
wl_pointer_leave(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    struct wl_surface *surface
) {
    (void) data;
    (void) wl_pointer;
    (void) serial;
    (void) surface;

    return;
}

static void
set_window_colors(uint32 *buffer, int x, int y) {
    uint32 alpha = w.alpha << 6*4;
    for (int j = 0; j < w.height; j += 1) {
        for (int i = 0; i < w.width; i += 1) {
            if (i < x) {
                if (j < y)
                    buffer[j*w.width + i] = alpha | w.pallete[0];
                else
                    buffer[j*w.width + i] = alpha | w.pallete[1];
            } else {
                if (j < y)
                    buffer[j*w.width + i] = alpha | w.pallete[2];
                else
                    buffer[j*w.width + i] = alpha | w.pallete[3];
            }
        }
    }
    wl_surface_attach(w.surface, w.buffer, 0, 0);
    wl_surface_damage(w.surface, 0, 0, w.width, w.height);
    wl_surface_commit(w.surface);
    w.dirty = false;
    return;
}

static void
wl_pointer_motion(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y
) {
    (void) data;
    (void) wl_pointer;
    (void) time;

    w.x = wl_fixed_to_int(surface_x);
    w.y = wl_fixed_to_int(surface_y);

    set_window_colors(w.draw_buffer, w.x, w.y);
    return;
}

static void
wl_pointer_button(
    void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state
) {
    struct wl_seat *seat = data;
    (void) pointer;
    (void) time;

    if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
        switch (button) {
        case BTN_LEFT:
            xdg_toplevel_move(w.xdg_toplevel, seat, serial);
            break;
        case BTN_RIGHT: {
            uint32 resize_type = XDG_TOPLEVEL_RESIZE_EDGE_NONE;
            if (w.y < (w.height / 2))
                resize_type |= XDG_TOPLEVEL_RESIZE_EDGE_TOP;
            else
                resize_type |= XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
            if (w.x < (w.width / 2))
                resize_type |= XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
            else
                resize_type |= XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
            xdg_toplevel_resize(w.xdg_toplevel, seat, serial, resize_type);
            break;
        }
        case BTN_MIDDLE:
            xdg_toplevel_configure(NULL, w.xdg_toplevel, WIDTH, HEIGHT, NULL);
            set_window_colors(w.draw_buffer, w.x, w.y);
            break;
        }
    }

    return;
}

static void
wl_pointer_axis(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value
) {
    (void) data;
    (void) wl_pointer;
    (void) time;
    (void) axis;
    (void) value;

    return;
}

static void
wl_pointer_frame(
    void *data,
    struct wl_pointer *wl_pointer
) {
    (void) data;
    (void) wl_pointer;

    return;
}

static void
wl_pointer_axis_source(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis_source
) {
    (void) data;
    (void) wl_pointer;
    (void) axis_source;

    return;
}

static void
wl_pointer_axis_stop(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    uint32_t axis
) {
    (void) data;
    (void) wl_pointer;
    (void) time;
    (void) axis;

    return;
}

static void
wl_pointer_axis_discrete(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    int32_t discrete
) {
    (void) data;
    (void) wl_pointer;
    (void) axis;
    (void) discrete;

    return;
}

static void
wl_pointer_axis_value120(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    int32_t value120
) {
    (void) data;
    (void) wl_pointer;
    (void) axis;
    (void) value120;

    return;
}

static void
wl_pointer_axis_relative_direction(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t axis,
    uint32_t direction
) {
    (void) data;
    (void) wl_pointer;
    (void) axis;
    (void) direction;

    return;
}

static struct wl_pointer_listener
wl_pointer_listener = {
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
    uint32_t format,
    int32_t fd,
    uint32_t size
) {
    char *keymap_name;
    (void) data;
    (void) wl_keyboard;
    (void) format;
    (void) size;

    keymap_name = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (keymap_name != MAP_FAILED) {
        xkb_keymap_unref(k.xkb_keymap);
        k.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        k.xkb_keymap = 
            xkb_keymap_new_from_string(k.xkb_context, keymap_name,
                                       XKB_KEYMAP_FORMAT_TEXT_V1,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);
        xkb_state_unref(k.xkb_state);
        k.xkb_state = xkb_state_new(k.xkb_keymap);
        munmap(keymap_name, size);
    }
    close(fd);

    return;
}

static void
wl_keyboard_enter(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    struct wl_surface *surface,
    struct wl_array *keys
) {
    (void) data;
    (void) wl_keyboard;
    (void) serial;
    (void) surface;
    (void) keys;

    return;
}

static void
wl_keyboard_leave(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    struct wl_surface *surface
) {
    (void) data;
    (void) wl_keyboard;
    (void) serial;
    (void) surface;

    return;
}

static void
wl_keyboard_key(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state
) {
    xkb_keycode_t keycode;
    xkb_keysym_t keysym;
    xkb_keysym_t utf32;
    (void) data;
    (void) wl_keyboard;
    (void) serial;
    (void) time;

    keycode = key + 8; // Wayland keycodes start from 8
    keysym = xkb_state_key_get_one_sym(k.xkb_state, keycode);
    utf32 = xkb_keysym_to_utf32(keysym);

    if (state) {
        switch (utf32) {
        case XKB_KEY_q:
            running = false;
            break;
        case XKB_KEY_u:
            memcpy(w.pallete, red, sizeof (red));
            break;
        case XKB_KEY_i:
            memcpy(w.pallete, green, sizeof (red));
            break;
        case XKB_KEY_o:
            memcpy(w.pallete, blue, sizeof (red));
            break;
        case XKB_KEY_p:
            memcpy(w.pallete, gray, sizeof (red));
            break;
        case XKB_KEY_r:
            memcpy(w.pallete, palletes[rand() % LENGTH(palletes)], sizeof (red));
            break;
        case XKB_KEY_k:
            if (w.alpha >= 0x0F)
                w.alpha -= 0x0F;
            break;
        case XKB_KEY_l:
            if (w.alpha <= 0xF0)
                w.alpha += 0x0F;
            break;
        }
        set_window_colors(w.draw_buffer, w.x, w.y);
    }

    return;
}

static void
wl_keyboard_modifiers(
    void *data,
    struct wl_keyboard *wl_keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group
) {
    (void) data;
    (void) wl_keyboard;
    (void) serial;
    (void) mods_depressed;
    (void) mods_latched;
    (void) mods_locked;
    (void) group;

    return;
}

static void
wl_keyboard_repeat_info(
    void *data,
    struct wl_keyboard *wl_keyboard,
    int32_t rate,
    int32_t delay
) {
    (void) data;
    (void) wl_keyboard;
    (void) rate;
    (void) delay;

    return;
}

static struct wl_keyboard_listener
wl_keyboard_listener = {
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
    uint32_t capabilities
) {
    (void) data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        w.wl_pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(w.wl_pointer, &wl_pointer_listener, seat);
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        w.wl_keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(w.wl_keyboard, &wl_keyboard_listener, seat);
    }
}

static void
seat_name(
    void *data,
    struct wl_seat *wl_seat,
    const char *name
) {
    (void) data;
    (void) wl_seat;
    (void) name;

    return;
}

static struct wl_seat_listener
seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};

static void
xdg_wm_base_listener_ping(void *data,
                      struct xdg_wm_base *wm_base,
                      uint32_t serial) {
    (void) data;

    xdg_wm_base_pong(wm_base, serial);
    return;
}

static struct xdg_wm_base_listener
xdg_wm_base_listener = {
    .ping = xdg_wm_base_listener_ping,
};

static void
wl_registry_global(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version
) {
    (void) data;
    (void) version;

    if (strcmp(interface, wl_shm_interface.name) == 0) {
        w.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        struct wl_seat *seat =
            wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
        w.compositor = wl_registry_bind(registry, name,
            &wl_compositor_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        w.xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    }

    return;
}

static void
wl_registry_global_remove(
    void *data,
    struct wl_registry *registry,
    uint32_t name
) {
    (void) data;
    (void) registry;
    (void) name;

    return;
}

static struct wl_registry_listener
wl_registry_listener = {
    .global = wl_registry_global,
    .global_remove = wl_registry_global_remove,
};

void resize_window(int width, int height) {
    int stride;

    if (width == 0 || height == 0)
        return;
    w.width = MAX(width, WIDTH_MIN);
    w.height = MAX(height, HEIGHT_MIN);
    w.width = MIN(w.width, WIDTH_MAX);
    w.height = MIN(w.height, HEIGHT_MAX);

    stride = w.width * 4;
    if (w.buffer)
        wl_buffer_destroy(w.buffer);
    w.buffer = wl_shm_pool_create_buffer(w.shm_pool, 0,
                                         w.width, w.height, stride,
                                         WL_SHM_FORMAT_ARGB8888);
    w.dirty = true;
    return;
}

static void create_buffer(void) {
    int fd;
	int retries = 100;

    w.alloc_size = 4 * WIDTH_MAX * HEIGHT_MAX;

	do {
        char name[] = "/hello-wayland-XXXXXX";
        char *buf = name + strlen(name) - 6;
        struct timespec ts;
        long r;
        clock_gettime(CLOCK_REALTIME, &ts);
        r = ts.tv_nsec;
        for (int i = 0; i < 6; ++i) {
            buf[i] = 'A'+(r&15)+(r&16)*2;
            r >>= 5;
        }

		retries -= 1;
		fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd < 0) {
            fprintf(stderr, "Error opening shared memory object: %s.\n",
                            strerror(errno));
        } else {
			shm_unlink(name);
			break;
		}
        fd = -1;
	} while (retries > 0 && errno == EEXIST);

	if (fd < 0) {
        fprintf(stderr, "Error creating shared memory file: %s.\n",
                        strerror(errno));
        exit(EXIT_FAILURE);
    }

	if (ftruncate(fd, w.alloc_size) < 0) {
        fprintf(stderr, "Error truncating shared memory file: %s.\n",
                        strerror(errno));
		close(fd);
        exit(EXIT_FAILURE);
	}

    w.draw_buffer = mmap(NULL, (usize) w.alloc_size,
                         PROT_READ | PROT_WRITE, MAP_SHARED,
                         fd, 0);
    if (w.draw_buffer == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %m\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    w.shm_pool = wl_shm_create_pool(w.shm, fd, w.alloc_size);

    resize_window(WIDTH, HEIGHT);
    memset(w.draw_buffer, 0xFF, (usize) w.alloc_size);
    return;
}

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    memset(&w, 0, sizeof (*(&w)));
    memset(w.pallete, 0xFF, sizeof (w.pallete));

    if ((w.display = wl_display_connect(NULL)) == NULL) {
        fprintf(stderr, "Error conecting to display.\n");
        exit(EXIT_FAILURE);
    }

    w.registry = wl_display_get_registry(w.display);
    wl_registry_add_listener(w.registry, &wl_registry_listener, NULL);
    wl_display_roundtrip(w.display);

    if (w.shm == NULL || w.compositor == NULL || w.xdg_wm_base == NULL) {
        fprintf(stderr, "Error: no wl_shm, wl_compositor, or xdg_wm_base support.\n");
        exit(EXIT_FAILURE);
    }

    create_buffer();
    w.alpha = 0xCC;

    w.surface = wl_compositor_create_surface(w.compositor);
    w.xdg_surface = xdg_wm_base_get_xdg_surface(w.xdg_wm_base, w.surface);
    w.xdg_toplevel = xdg_surface_get_toplevel(w.xdg_surface);

    xdg_toplevel_set_min_size(w.xdg_toplevel, WIDTH_MIN, HEIGHT_MIN);
    xdg_toplevel_set_max_size(w.xdg_toplevel, WIDTH_MAX, HEIGHT_MAX);
    xdg_toplevel_set_title(w.xdg_toplevel, "wayland-play");

    xdg_surface_add_listener(w.xdg_surface, &xdg_surface_listener, NULL);
    xdg_toplevel_add_listener(w.xdg_toplevel, &xdg_toplevel_listener, NULL);
    xdg_wm_base_add_listener(w.xdg_wm_base, &xdg_wm_base_listener, NULL);

    wl_surface_commit(w.surface);
    wl_display_roundtrip(w.display);

    wl_surface_attach(w.surface, w.buffer, 0, 0);
    wl_surface_commit(w.surface);

    do {
        int dispatch = wl_display_dispatch(w.display);
        if (dispatch == -1)
            break;
    } while (running);

    xkb_state_unref(k.xkb_state);
    xkb_keymap_unref(k.xkb_keymap);
    xkb_context_unref(k.xkb_context);
    xdg_toplevel_destroy(w.xdg_toplevel);
    xdg_surface_destroy(w.xdg_surface);
    wl_surface_destroy(w.surface);
    wl_buffer_destroy(w.buffer);

    printf("========== wayland-play: normal exit ====================");
    return 0;
}
