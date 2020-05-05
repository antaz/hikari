#if !defined(HIKARI_POSITION_CONFIG_H)
#define HIKARI_POSITION_CONFIG_H

#include <stdbool.h>

struct hikari_position_config {
  bool explicit_position;

  int x;
  int y;
};

void
hikari_position_config_init(struct hikari_position_config *position_config);

#endif
