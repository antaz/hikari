#if !defined(HIKARI_POINTER_H)
#define HIKARI_POINTER_H

#include <wlr/types/wlr_input_device.h>

struct hikari_pointer_config;

struct hikari_pointer {
  struct wl_list server_pointers;

  struct wlr_input_device *device;

  struct wl_listener destroy;
};

void
hikari_pointer_init(
    struct hikari_pointer *pointer, struct wlr_input_device *device);

void
hikari_pointer_fini(struct hikari_pointer *pointer);

void
hikari_pointer_configure(struct hikari_pointer *pointer,
    struct hikari_pointer_config *pointer_config);

#endif
