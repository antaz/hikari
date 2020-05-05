#if !defined(HIKARI_OUTPUT_CONFIG_H)
#define HIKARI_OUTPUT_CONFIG_H

#include <stdbool.h>

#include <wayland-util.h>

#include <hikari/position_config.h>

enum hikari_background_fit {
  HIKARI_BACKGROUND_CENTER,
  HIKARI_BACKGROUND_STRETCH,
  HIKARI_BACKGROUND_TILE
};

struct hikari_output_config {
  struct wl_list link;

  char *output_name;
  char *background;

  struct hikari_position_config position;

  enum hikari_background_fit background_fit;
};

void
hikari_output_config_init(struct hikari_output_config *output_config,
    const char *output_name,
    char *background,
    enum hikari_background_fit background_fit,
    struct hikari_position_config *position_config);

void
hikari_output_config_fini(struct hikari_output_config *output_config);

#endif
