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

#if !defined(WAYLAND_PLAY_H)
#define WAYLAND_PLAY_H

#include <linux/input-event-codes.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "xdg-shell-client-protocol.h"
#include "cbase.h"

#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 512
#define WINDOW_WIDTH_MIN 128
#define WINDOW_HEIGHT_MIN 128
#define WINDOW_WIDTH_MAX 1920
#define WINDOW_HEIGHT_MAX 1080

#define PALETTE_COLORS 4
#define SHM_NAME_RANDOM_BYTES 6
#define SHM_CREATE_RETRIES 100
#define BYTES_PER_PIXEL 4
#define WAYLAND_KEYCODE_OFFSET 8
#define ALPHA_SHIFT 24
#define ALPHA_STEP 0x0F
#define ALPHA_MIN 0x00
#define ALPHA_MAX 0xFF
#define ALPHA_DEFAULT 0xCC

typedef struct Keyboard {
    struct xkb_context *xkb_context;
    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;
} Keyboard;

typedef struct Window {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_buffer *buffer;
    struct wl_shm *shm;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_wm_base *xdg_wm_base;
    struct xdg_toplevel *xdg_toplevel;
    struct wl_shm_pool *shm_pool;
    struct wl_pointer *wl_pointer;
    struct wl_keyboard *wl_keyboard;

    uint32 *draw_buffer;

    int32 width;
    int32 height;
    int32 alloc_size;
    int32 x;
    int32 y;

    uint32 palette[PALETTE_COLORS];
    uint32 alpha;
    bool dirty;
} Window;

#endif /* WAYLAND_PLAY_H */
