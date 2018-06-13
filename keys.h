#ifndef KEYS_H
#define KEYS_H

#include <xcb/xcb.h>

void grab_keys(void);
void key_press(xcb_key_press_event_t *e);

#endif
