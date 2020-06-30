#if !defined(HIKARI_KEYBOARD_CONFIG_H)
#define HIKARI_KEYBOARD_CONFIG_H

#include <stdbool.h>
#include <ucl.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include <hikari/option.h>

struct hikari_xkb_config {
  HIKARI_OPTION(rules, char *);
  HIKARI_OPTION(model, char *);
  HIKARI_OPTION(layout, char *);
  HIKARI_OPTION(variant, char *);
  HIKARI_OPTION(options, char *);
};

HIKARI_OPTION_FUNS(xkb, rules, char *);
HIKARI_OPTION_FUNS(xkb, model, char *);
HIKARI_OPTION_FUNS(xkb, layout, char *);
HIKARI_OPTION_FUNS(xkb, variant, char *);
HIKARI_OPTION_FUNS(xkb, options, char *);

enum hikari_xkb_type { HIKARI_XKB_TYPE_KEYMAP, HIKARI_XKB_TYPE_RULES };

struct hikari_xkb {
  enum hikari_xkb_type type;

  union {
    struct hikari_xkb_config rules;
    struct xkb_keymap *keymap;
  } value;
};

struct hikari_keyboard_config {
  struct wl_list link;

  char *keyboard_name;

  struct hikari_xkb xkb;

  HIKARI_OPTION(repeat_delay, int);
  HIKARI_OPTION(repeat_rate, int);
};

HIKARI_OPTION_FUNS(keyboard, repeat_delay, int);
HIKARI_OPTION_FUNS(keyboard, repeat_rate, int);

void
hikari_keyboard_config_init(
    struct hikari_keyboard_config *keyboard_config, const char *keyboard_name);

void
hikari_keyboard_config_fini(struct hikari_keyboard_config *keyboard_config);

void
hikari_keyboard_config_default(struct hikari_keyboard_config *keyboard_config);

void
hikari_keyboard_config_merge(struct hikari_keyboard_config *keyboard_config,
    struct hikari_keyboard_config *default_config);

bool
hikari_keyboard_config_parse(struct hikari_keyboard_config *keyboard_config,
    const ucl_object_t *keyboard_config_obj);

bool
hikari_keyboard_config_compile_keymap(
    struct hikari_keyboard_config *keyboard_config);

#endif
