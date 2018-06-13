#ifndef TINTWM_H
#define TINTWM_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <xcb/xcb.h>

#include "layouts.h"

#define DIE(x) do { fputs(x, stderr); exit(EXIT_FAILURE); } while (0)
#define DIE_F(x, ...) do { fprintf(stderr, x, ##__VA_ARGS__); exit(EXIT_FAILURE); } while (0)

struct client {
	xcb_window_t window;
	int16_t x, y;
	uint16_t w, h;
	struct client *next;
	struct client *focus_next;
};

// Global variables
extern xcb_connection_t *dpy;
extern xcb_screen_t *screen;
extern struct client *focus, *master;
extern bool running;
extern uint16_t sw, sh;

// General utilities
void move_resize(struct client *c, int16_t x, int16_t y, uint16_t w, uint16_t h);
void focus_client(struct client *c);

#endif
