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

  pointer_config->accel.configured = false;
  pointer_config->disable_while_typing.configured = false;
  pointer_config->natural_scrolling.configured = false;
  pointer_config->scroll_button.configured = false;
  pointer_config->scroll_method.configured = false;
  pointer_config->tap.configured = false;
  pointer_config->tap_drag.configured = false;
  pointer_config->tap_drag_lock.configured = false;
}

void
hikari_pointer_config_fini(struct hikari_pointer_config *pointer_config)
{
  hikari_free(pointer_config->name);
}
