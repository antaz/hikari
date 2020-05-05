#include <hikari/position_config.h>

void
hikari_position_config_init(struct hikari_position_config *position_config)
{
  position_config->explicit_position = false;
  position_config->x = 0;
  position_config->y = 0;
}
