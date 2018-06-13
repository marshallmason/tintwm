// This is where you create tiling layouts
// By default, only one layout is defined: verticle
// It's easy to implement your own, using verticle as an example
// Just loop through the clients and call move_resize
// Add it to the switch statement in arrange()
// Add an entry to enum layout in layouts.h

#include <stdint.h>

#include "tintwm.h"
#include "layouts.h"

enum layout layout = VERTICLE;

static uint16_t
nclients(void)
{
	uint16_t result = 0;
	for (struct client *c = master; c; c = c->next)
		result++;
	return result;
}

static void
verticle(void)
{
	const uint16_t n = nclients();
	if (!n) return;

	int i = -1;
	for (struct client *c = master; c; c = c->next, i++) {
		if (i < 0)
			move_resize(c, 0, 0, n == 1 ? sw : sw / 2, sh);
		else
			move_resize(c, sw / 2, i * (sh / (n - 1)), sw / 2, sh / (n - 1));
	}
}

void
arrange(void)
{
	switch (layout) {
	case VERTICLE: verticle(); break;
	}
}
