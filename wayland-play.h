/* This file is part of wayland-play.
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

#ifndef WAYLAND_PLAY_H
#define WAYLAND_PLAY_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <linux/input-event-codes.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "xdg-shell-client-protocol.h"

#define MIN(a,b) (a) < (b) ? (a) : (b)
#define MAX(a,b) (a) > (b) ? (a) : (b)
#define LENGTH(X) (int) (sizeof (X) / sizeof (*X))

#define WIDTH 512
#define HEIGHT 512
#define WIDTH_MIN 128
#define HEIGHT_MIN 128
#define WIDTH_MAX 1920
#define HEIGHT_MAX 1080

#ifndef INTEGERS
#define INTEGERS
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t usize;
typedef ssize_t isize;
#endif

struct Keyboard {
    struct xkb_context *xkb_context;
    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;
};

struct Window {
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

    int width;
    int height;
    int size;
    int alloc_size;
    int dirty;

    uint32 pallete[4];
    uint32 alpha;
    void *draw_buffer;
    int x;
    int y;
};

extern uint32 red[4];
extern uint32 green[4];
extern uint32 blue[4];
extern uint32 gray[4];
extern uint32 *palletes[4];

#endif /* WAYLAND_PLAY_H */
