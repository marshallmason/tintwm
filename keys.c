// This is where you create keybindings
// Just add an entry to the keys[] array in grab_keys
// and provide the action in the appropriate switch case in key_press

#include <X11/keysym.h>
#include <xcb/xcb_keysyms.h>

#include "tintwm.h"

#define WIN XCB_MOD_MASK_4
#define ALT XCB_MOD_MASK_1
#define CONTROL XCB_MOD_MASK_CONTROL
#define SHIFT XCB_MOD_MASK_SHIFT

static xcb_key_symbols_t *keysyms;

void
grab_keys(void)
{
	struct key { uint16_t mod; xcb_keysym_t key; };
	struct key keys[] = {
		{ WIN|SHIFT, XK_q },
	};

	keysyms = xcb_key_symbols_alloc(dpy);
	if (!keysyms) DIE("cannot allocate keysyms\n");

	#define LENGTH(x) (sizeof(x)/sizeof(*x))
	for (size_t i = 0; i < LENGTH(keys); i++) {
		xcb_keycode_t *code = xcb_key_symbols_get_keycode(keysyms, keys[i].key);
		if (!code) continue;

		xcb_grab_key(dpy, 1, screen->root, keys[i].mod, *code,
		    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

		free(code);
	}
}

void
key_press(xcb_key_press_event_t *e)
{
	const xcb_keysym_t key = xcb_key_symbols_get_keysym(keysyms, e->detail, 0);

	switch (e->state) { // modifier
	case WIN|SHIFT:
		switch (key) {
		case XK_q: running = false; break;
		}
		break;
	}
}
