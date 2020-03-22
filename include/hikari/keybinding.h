#if !defined(HIKARI_KEYBINDING_H)
#define HIKARI_KEYBINDING_H

#include <stdint.h>

struct hikari_action;

struct hikari_keybinding {
  uint32_t keycode;
  struct hikari_action *action;
};

#endif
