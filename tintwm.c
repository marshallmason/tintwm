#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include <xcb/xcb.h>

#include "tintwm.h"
#include "keys.h"
#include "layouts.h"

xcb_connection_t *dpy;
xcb_screen_t *screen;
struct client *focus, *master;
bool running = true;
uint16_t sw, sh;

void
move_resize(struct client *c, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
	if (!c) return;

	uint16_t mask = 0;
	uint32_t values[4] = {0};
	int i = 0;

	#define SET_GEOMETRY(field, xcb_field, force_set) \
		if (force_set || field != c->field) { \
			c->field = field; \
			mask |= XCB_CONFIG_WINDOW_##xcb_field; \
			values[i++] = field; \
		}

	const bool force = (c->w == 0);
	SET_GEOMETRY(x, X, force);
	SET_GEOMETRY(y, Y, force);
	SET_GEOMETRY(w, WIDTH, force);
	SET_GEOMETRY(h, HEIGHT, force);

	if (!mask) return;
	xcb_configure_window(dpy, c->window, mask, values);
}

static void
add_focus(struct client *c)
{
	if (c == focus) return;

	// Detach client from focus stack
	struct client *prev = focus;
	for (; prev && prev->focus_next != c; prev = prev->focus_next);
	if (prev) prev->focus_next = c->focus_next;

	// Push client to top of focus stack
	c->focus_next = focus;
	focus = c;
}

static void
raise_window(xcb_window_t window)
{
	const uint32_t values[] = { XCB_STACK_MODE_ABOVE };
	xcb_configure_window(dpy, window, XCB_CONFIG_WINDOW_STACK_MODE, values);
	xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_PARENT, window, XCB_CURRENT_TIME);
}

void
focus_client(struct client *c)
{
	if (!c) return;
	add_focus(c);
	raise_window(c->window);
}

static void
activate_wm(void)
{
	const xcb_event_mask_t event_mask[] = {
	    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
	    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY };

	const xcb_void_cookie_t attr = xcb_change_window_attributes_checked(
	    dpy, screen->root, XCB_CW_EVENT_MASK, event_mask);

	xcb_generic_error_t *err = xcb_request_check(dpy, attr);
	if (err) DIE("another window manager is running\n");
}

static void
grab_buttons(void)
{
	xcb_grab_button(dpy, 1, screen->root, XCB_EVENT_MASK_BUTTON_PRESS | 
	    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_SYNC, 
	    XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE, XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);
}

static void
setup(void)
{
	int screen_num;
	dpy = xcb_connect(NULL, &screen_num);
	if (xcb_connection_has_error(dpy)) exit(1);

	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(dpy));

	for (int i = 0; i < screen_num; i++)
		xcb_screen_next(&iter);

	screen = iter.data;

	master = focus = NULL;
	sw = screen->width_in_pixels;
	sh = screen->height_in_pixels;

	activate_wm();
	grab_keys();
	grab_buttons();
}

static struct client *
find_client(xcb_window_t window)
{
	for (struct client *c = master; c; c = c->next)
		if (c->window == window) return c;
	return NULL;
}

static struct client *
add_window(xcb_window_t window)
{
	struct client *c = find_client(window);
	if (c) return c;

	c = calloc(1, sizeof(struct client));
	if (!c) DIE("unable to allocate memory\n");

	c->window = window;

	struct client *last = master;
	for (; last && last->next; last = last->next);
	if (last)
		last->next = c;
	else
		master = c;

	return c;
}

static void
map_request(xcb_map_request_event_t *e)
{
	struct client *c = add_window(e->window);
	add_focus(c);
	arrange();
	xcb_map_window(dpy, e->window);
	raise_window(e->window);
}

static void
button_press(xcb_button_press_event_t *e)
{
	struct client *c = find_client(e->child);
	if (c) focus_client(c);
	xcb_allow_events(dpy, XCB_ALLOW_REPLAY_POINTER, e->time);
}

static void
configure_request(xcb_configure_request_event_t *e)
{
	if (find_client(e->window)) return;

	uint32_t values[6] = {0};
	int i = 0;

	#define SET_VALUE(lowercase, uppercase) \
		if (e->value_mask & XCB_CONFIG_WINDOW_##uppercase) \
			values[i++] = e->lowercase;

	SET_VALUE(x, X);
	SET_VALUE(y, Y);
	SET_VALUE(width, WIDTH);
	SET_VALUE(height, HEIGHT);
	SET_VALUE(sibling, SIBLING);
	SET_VALUE(stack_mode, STACK_MODE);

	xcb_configure_window(dpy, e->window, e->value_mask, values);
}

static void
destroy_notify(xcb_destroy_notify_event_t *e)
{
	struct client *c = find_client(e->window);
	if (!c) return;

	// Detach from clients list
	struct client *prev = master;
	if (!master->next) { // only one client
		master = NULL;
	} else if (master == c) { // first client
		master = c->next;
	} else { // somewhere in the middle, or end
		for (; prev && prev->next != c; prev = prev->next);
		prev->next = c->next;
	}

	// Detach from focus stack
	if (!focus->focus_next) { // only one client
		focus = NULL;
	} else if (focus == c) { // first client
		focus = c->focus_next;
	} else { // somewhere in the middle, or end
		prev = focus;
		for (; prev && prev->focus_next != c; prev = prev->focus_next);
		prev->focus_next = c->focus_next;
	}

	free(c);

	arrange();
	focus_client(focus);
}

static void
run(void)
{
	while (running) {
		xcb_flush(dpy);
		xcb_generic_event_t *ev = xcb_wait_for_event(dpy);
		switch (ev->response_type & ~0x80) {
		case XCB_KEY_PRESS:
			key_press((xcb_key_press_event_t *) ev);
			break;
		case XCB_BUTTON_PRESS:
			button_press((xcb_button_press_event_t *) ev);
			break;
		case XCB_CONFIGURE_REQUEST:
			configure_request((xcb_configure_request_event_t *) ev);
			break;
		case XCB_MAP_REQUEST:
			map_request((xcb_map_request_event_t *) ev);
			break;
		case XCB_DESTROY_NOTIFY:
			destroy_notify((xcb_destroy_notify_event_t *) ev);
			break;
		}
		free(ev);
		if (xcb_connection_has_error(dpy))
			DIE("the server closed the connection\n");
	}
}

int
main(void)
{
	setup();
	run();
	return EXIT_SUCCESS;
}
