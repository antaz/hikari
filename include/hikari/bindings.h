#if !defined(HIKARI_BINDINGS_H)
#define HIKARI_BINDINGS_H

#include <stdint.h>

struct hikari_keybinding;

struct hikari_modifier_bindings {
  int nbindings;
  struct hikari_keybinding *bindings;
};

struct hikari_bindings {
  struct hikari_modifier_bindings keyboard[256];
  struct hikari_modifier_bindings mouse[256];
};

void
hikari_bindings_init(struct hikari_bindings *bindings);

void
hikari_bindings_fini(struct hikari_bindings *bindings);

#endif
