#include <hikari/pointer_config.h>

#include <hikari/memory.h>

void
hikari_pointer_config_fini(struct hikari_pointer_config *pointer_config)
{
  hikari_free(pointer_config->name);
}
