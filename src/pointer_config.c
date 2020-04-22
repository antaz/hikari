#include <hikari/pointer_config.h>

#include <string.h>

#include <linux/input-event-codes.h>

#include <hikari/memory.h>

void
hikari_pointer_config_init(
    struct hikari_pointer_config *pointer_config, const char *name)
{
  size_t keylen = strlen(name);

  pointer_config->name = hikari_malloc(keylen + 1);
  strcpy(pointer_config->name, name);
  pointer_config->accel = 0;
  pointer_config->scroll_method = LIBINPUT_CONFIG_SCROLL_NO_SCROLL;
  pointer_config->scroll_button = 0;
}

void
hikari_pointer_config_fini(struct hikari_pointer_config *pointer_config)
{
  hikari_free(pointer_config->name);
}
