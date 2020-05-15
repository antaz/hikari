#if !defined(HIKARI_OUTPUT_CONFIG_H)
#define HIKARI_OUTPUT_CONFIG_H

#include <stdbool.h>

#include <wayland-util.h>

#include <hikari/option.h>
#include <hikari/position_config.h>

enum hikari_background_fit {
  HIKARI_BACKGROUND_CENTER,
  HIKARI_BACKGROUND_STRETCH,
  HIKARI_BACKGROUND_TILE
};

struct hikari_output_config {
  struct wl_list link;

  char *output_name;

  HIKARI_OPTION(background, char *);
  HIKARI_OPTION(background_fit, enum hikari_background_fit);
  HIKARI_OPTION(position, struct hikari_position_config);
};

void
hikari_output_config_init(
    struct hikari_output_config *output_config, const char *output_name);

void
hikari_output_config_fini(struct hikari_output_config *output_config);

void
hikari_output_config_merge(struct hikari_output_config *output_config,
    struct hikari_output_config *default_config);

HIKARI_OPTION_FUNS(output, background, char *);
HIKARI_OPTION_FUNS(output, background_fit, enum hikari_background_fit);
HIKARI_OPTION_FUNS(output, position, struct hikari_position_config);

#endif
