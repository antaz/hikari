#if !defined(HIKARI_BINDING_H)
#define HIKARI_BINDING_H

#include <stdint.h>

struct hikari_action;

struct hikari_binding {
  uint32_t keycode;
  struct hikari_action *action;
};

#endif
