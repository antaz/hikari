#if !defined(HIKARI_OUTPUT_CONFIG_H)
#define HIKARI_OUTPUT_CONFIG_H

#include <wayland-util.h>

struct hikari_output_config {
  struct wl_list link;

  char *output_name;
  char *background;
};

void
hikari_output_config_init(struct hikari_output_config *output_config,
    const char *output_name,
    char *background);

void
hikari_output_config_fini(struct hikari_output_config *output_config);

#endif
