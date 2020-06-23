#if !defined(HIKARI_BINDING_CONFIG)
#define HIKARI_BINDING_CONFIG

#include <stdbool.h>
#include <stdint.h>

#include <xkbcommon/xkbcommon.h>

#include <wayland-util.h>

#include <hikari/action.h>

enum hikari_binding_key_type {
  HIKARI_ACTION_BINDING_KEY_KEYCODE,
  HIKARI_ACTION_BINDING_KEY_KEYSYM
};

struct hikari_binding_config_key {
  enum hikari_binding_key_type type;

  uint8_t modifiers;

  union {
    xkb_keysym_t keysym;
    uint32_t keycode;
  } value;
};

struct hikari_binding_config {
  struct wl_list link;

  struct hikari_binding_config_key key;
  struct hikari_action action;
};

bool
hikari_binding_config_key_parse(
    struct hikari_binding_config_key *binding_key, const char *str);

bool
hikari_binding_config_button_parse(
    struct hikari_binding_config_key *binding_key, const char *str);

#endif
