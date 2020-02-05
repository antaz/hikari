#if !defined(HIKARI_UNLOCKER_H)
#define HIKARI_UNLOCKER_H

#include <wayland-server-core.h>

void
hikari_unlocker_init(void);

void
hikari_unlocker_fini(void);

void
hikari_unlocker_start(void);

void
hikari_unlocker_key_handler(struct wl_listener *listener, void *data);

#endif
