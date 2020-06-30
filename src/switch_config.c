#include <hikari/switch_config.h>

void
hikari_switch_config_fini(struct hikari_switch_config *switch_config)
{
  free(switch_config->switch_name);
}
