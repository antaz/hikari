#if !defined(HIKARI_SWITCH_CONFIG_H)
#define HIKARI_SWITCH_CONFIG_H

#include <hikari/action.h>

struct hikari_switch_config {
  struct wl_list link;

  char *switch_name;
  struct hikari_action action;
};

void
hikari_switch_config_fini(struct hikari_switch_config *switch_config);

#endif
