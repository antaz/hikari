#if !defined(HIKARI_BINDINGS_H)
#define HIKARI_BINDINGS_H

#include <stdint.h>

struct hikari_keybinding;

struct hikari_modifier_bindings {
  uint8_t nbindings;
  struct hikari_keybinding *bindings;
};

struct hikari_input_bindings {
  struct hikari_modifier_bindings pressed[256];
  struct hikari_modifier_bindings released[256];
};

struct hikari_bindings {
  struct hikari_input_bindings keyboard;
  struct hikari_input_bindings mouse;
};

void
hikari_bindings_init(struct hikari_bindings *bindings);

void
hikari_bindings_fini(struct hikari_bindings *bindings);

#endif
