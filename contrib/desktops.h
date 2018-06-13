#include "../layouts.h"

#define DEFAULT_ID '1'

struct desktop {
	char id;
	struct client *master;
	struct client *focus;
	enum layout layout;
	struct desktop *next;
};

static struct desktop *firstdt, *currentdt;

static void
load_desktop(struct desktop *dt)
{
	master = dt->master;
	focus = dt->focus;
	layout = dt->layout;

	currentdt = dt;
	if (!firstdt) firstdt = currentdt;
}

static struct desktop *
create_desktop(char id)
{
	struct desktop *dt = calloc(1, sizeof(struct desktop));
	if (!dt) DIE("unable to allocate memory\n");
	dt->id = id;

	struct desktop *last = firstdt;
	for (; last && last->next; last = last->next);
	if (last)
		last->next = dt;
	else
		firstdt = dt;

	return dt;
}

static void
save_desktop(void)
{
	if (!currentdt)
		currentdt = create_desktop(DEFAULT_ID);

	currentdt->master = master;
	currentdt->focus = focus;
	currentdt->layout = layout;
}

static void
remove_empty_desktop(void)
{
	if (!currentdt || !firstdt) return;

	if (currentdt == firstdt) {
		firstdt = firstdt->next;
	} else {
		struct desktop *prev = firstdt;
		for (; prev && prev->next != currentdt; prev = prev->next);
		prev->next = currentdt->next;
	}

	free(currentdt);
	currentdt = NULL;
}

static void
change_desktop(char id)
{
	if (currentdt && id == currentdt->id) return;
	if (!currentdt && id == DEFAULT_ID) return;

	// Find desktop
	struct desktop *dt = firstdt;
	for (; dt && dt->id != id; dt = dt->next);
	if (!dt) dt = create_desktop(id);

	// Show windows from new desktop
	for (struct client *c = dt->master; c; c = c->next)
		xcb_map_window(dpy, c->window);

	// Remove current windows from display
	for (struct client *c = master; c; c = c->next)
		xcb_unmap_window(dpy, c->window);

	// Change global data to target desktop
	if (master) // not empty
		save_desktop();
	else
		remove_empty_desktop();
	load_desktop(dt);

	focus_client(focus);
}
