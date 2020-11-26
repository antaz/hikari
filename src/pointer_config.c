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

  hikari_pointer_config_init_accel(pointer_config, 0);
  hikari_pointer_config_init_accel_profile(
      pointer_config, LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE);
  hikari_pointer_config_init_disable_while_typing(pointer_config, false);
  hikari_pointer_config_init_middle_emulation(
      pointer_config, LIBINPUT_CONFIG_MIDDLE_EMULATION_DISABLED);
  hikari_pointer_config_init_natural_scrolling(pointer_config, false);
  hikari_pointer_config_init_scroll_button(pointer_config, 0);
  hikari_pointer_config_init_scroll_method(pointer_config, 0);
  hikari_pointer_config_init_tap(pointer_config, false);
  hikari_pointer_config_init_tap_drag(pointer_config, false);
  hikari_pointer_config_init_tap_drag_lock(pointer_config, false);
}

void
hikari_pointer_config_fini(struct hikari_pointer_config *pointer_config)
{
  hikari_free(pointer_config->name);
}

void
hikari_pointer_config_merge(struct hikari_pointer_config *pointer_config,
    struct hikari_pointer_config *default_config)
{
#define MERGE(option)                                                          \
  hikari_pointer_config_merge_##option(pointer_config, default_config);
  MERGE(accel);
  MERGE(accel_profile);
  MERGE(disable_while_typing);
  MERGE(middle_emulation);
  MERGE(natural_scrolling);
  MERGE(scroll_button);
  MERGE(scroll_method);
  MERGE(tap);
  MERGE(tap_drag);
  MERGE(tap_drag_lock);
#undef MERGE
}
