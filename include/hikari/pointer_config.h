#if !defined(HIKARI_POINTER_CONFIG_H)
#define HIKARI_POINTER_CONFIG_H

#include <libinput.h>
#include <wayland-util.h>

struct hikari_pointer_config {
  struct wl_list link;

  char *name;

  double accel;
  enum libinput_config_scroll_method scroll_method;
  uint32_t scroll_button;
};

void
hikari_pointer_config_init(
    struct hikari_pointer_config *pointer_config, const char *name);

void
hikari_pointer_config_fini(struct hikari_pointer_config *pointer_config);

#endif
