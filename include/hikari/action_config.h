#if !defined(HIKARI_ACTION_CONFIG_H)
#define HIKARI_ACTION_CONFIG_H

#include <wayland-util.h>

struct hikari_action_config {
  struct wl_list link;

  char *action_name;
  char *command;
};

void
hikari_action_config_init(struct hikari_action_config *action_config,
    const char *action_name,
    const char *command);

void
hikari_action_config_fini(struct hikari_action_config *action_config);

#endif
