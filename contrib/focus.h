#include "../tintwm.h"

static void
focus_next(void)
{
	focus_client(focus->next ? focus->next : master);
}

static void
focus_prev(void)
{
	struct client *prev = master;
	for (; prev && prev->next != focus; prev = prev->next);
	if (!prev) // wrap around
		for (prev = master; prev && prev->next; prev = prev->next);
	focus_client(prev);
}

static void
focus_nth(int n)
{
	struct client *c = master;
	for (int i = 0; i < n; c = c->next, i++);
	focus_client(c);
}
