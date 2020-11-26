#if !defined(HIKARI_POINTER_CONFIG_H)
#define HIKARI_POINTER_CONFIG_H

#include <stdbool.h>

#include <libinput.h>
#include <wayland-util.h>

#include <hikari/option.h>

struct hikari_pointer_config {
  struct wl_list link;

  char *name;

  HIKARI_OPTION(accel, double);
  HIKARI_OPTION(accel_profile, enum libinput_config_accel_profile);
  HIKARI_OPTION(disable_while_typing, enum libinput_config_dwt_state);
  HIKARI_OPTION(middle_emulation, enum libinput_config_middle_emulation_state);
  HIKARI_OPTION(natural_scrolling, bool);
  HIKARI_OPTION(scroll_button, uint32_t);
  HIKARI_OPTION(scroll_method, enum libinput_config_scroll_method);
  HIKARI_OPTION(tap, enum libinput_config_tap_state);
  HIKARI_OPTION(tap_drag, enum libinput_config_drag_state);
  HIKARI_OPTION(tap_drag_lock, enum libinput_config_drag_lock_state);
};

void
hikari_pointer_config_init(
    struct hikari_pointer_config *pointer_config, const char *name);

void
hikari_pointer_config_fini(struct hikari_pointer_config *pointer_config);

void
hikari_pointer_config_merge(struct hikari_pointer_config *pointer_config,
    struct hikari_pointer_config *default_config);

HIKARI_OPTION_FUNS(pointer, accel, double)
HIKARI_OPTION_FUNS(pointer, accel_profile, enum libinput_config_accel_profile)
HIKARI_OPTION_FUNS(pointer, disable_while_typing, bool)
HIKARI_OPTION_FUNS(pointer, middle_emulation, bool);
HIKARI_OPTION_FUNS(pointer, natural_scrolling, bool)
HIKARI_OPTION_FUNS(pointer, scroll_button, uint32_t)
HIKARI_OPTION_FUNS(pointer, scroll_method, enum libinput_config_scroll_method)
HIKARI_OPTION_FUNS(pointer, tap, enum libinput_config_tap_state)
HIKARI_OPTION_FUNS(pointer, tap_drag, enum libinput_config_drag_state)
HIKARI_OPTION_FUNS(pointer, tap_drag_lock, enum libinput_config_drag_lock_state)

#endif
