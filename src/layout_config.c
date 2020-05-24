#include <hikari/layout_config.h>

#include <string.h>

#include <hikari/memory.h>
#include <hikari/split.h>

void
hikari_layout_config_init(struct hikari_layout_config *layout_config,
    char layout_register,
    struct hikari_split *split)
{
  layout_config->layout_register = layout_register;
  layout_config->split = split;
}

void
hikari_layout_config_fini(struct hikari_layout_config *layout_config)
{
  hikari_split_free(layout_config->split);
}
