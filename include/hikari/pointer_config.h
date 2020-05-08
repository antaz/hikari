#if !defined(HIKARI_POINTER_CONFIG_H)
#define HIKARI_POINTER_CONFIG_H

#include <stdbool.h>

#include <libinput.h>
#include <wayland-util.h>

#define CONFIGURATION(name, type)                                              \
  struct {                                                                     \
    bool configured;                                                           \
    type value;                                                                \
  } name

struct hikari_pointer_config {
  struct wl_list link;

  char *name;

  CONFIGURATION(accel, double);
  CONFIGURATION(disable_while_typing, enum libinput_config_dwt_state);
  CONFIGURATION(natural_scrolling, bool);
  CONFIGURATION(scroll_button, uint32_t);
  CONFIGURATION(scroll_method, enum libinput_config_scroll_method);
  CONFIGURATION(tap, enum libinput_config_tap_state);
  CONFIGURATION(tap_drag, enum libinput_config_drag_state);
  CONFIGURATION(tap_drag_lock, enum libinput_config_drag_lock_state);
};
#undef CONFIGURATION

void
hikari_pointer_config_init(
    struct hikari_pointer_config *pointer_config, const char *name);

void
hikari_pointer_config_fini(struct hikari_pointer_config *pointer_config);

#define CONFIGURE(name, type)                                                  \
                                                                               \
  static inline void hikari_pointer_config_set_##name(                         \
      struct hikari_pointer_config *pointer_config, type value)                \
  {                                                                            \
    pointer_config->name.configured = true;                                    \
    pointer_config->name.value = value;                                        \
  }                                                                            \
                                                                               \
  static inline bool hikari_pointer_config_has_##name(                         \
      struct hikari_pointer_config *pointer_config)                            \
  {                                                                            \
    return pointer_config->name.configured;                                    \
  }

CONFIGURE(accel, double)
CONFIGURE(disable_while_typing, bool)
CONFIGURE(natural_scrolling, bool)
CONFIGURE(scroll_button, uint32_t)
CONFIGURE(scroll_method, enum libinput_config_scroll_method)
CONFIGURE(tap, enum libinput_config_tap_state)
CONFIGURE(tap_drag, enum libinput_config_drag_state)
CONFIGURE(tap_drag_lock, enum libinput_config_drag_lock_state)
#undef CONFIGURE

#endif
