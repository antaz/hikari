#include <hikari/layout_config.h>

#include <string.h>

#include <hikari/memory.h>
#include <hikari/split.h>

void
hikari_layout_config_init(struct hikari_layout_config *layout_config,
    const char *layout_name,
    struct hikari_split *split)
{
  size_t layout_name_len = strlen(layout_name);
  layout_config->layout_name = hikari_malloc(layout_name_len + 1);

  strcpy(layout_config->layout_name, layout_name);
  layout_config->split = split;
}

void
hikari_layout_config_fini(struct hikari_layout_config *layout_config)
{
  hikari_free(layout_config->layout_name);
  hikari_split_fini(layout_config->split);
}
