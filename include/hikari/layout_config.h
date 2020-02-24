#if !defined(HIKARI_LAYOUT_CONFIG_H)
#define HIKARI_LAYOUT_CONFIG_H

#include <wayland-util.h>

struct hikari_split;

struct hikari_layout_config {
  struct wl_list link;

  char layout_register;
  struct hikari_split *split;
};

void
hikari_layout_config_init(struct hikari_layout_config *layout_config,
    char layout_register,
    struct hikari_split *split);

void
hikari_layout_config_fini(struct hikari_layout_config *layout_config);

#endif
