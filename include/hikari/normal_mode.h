#if !defined(HIKARI_NORMAL_MODE_H)
#define HIKARI_NORMAL_MODE_H

#include <hikari/mode.h>

struct hikari_keybinding;

struct hikari_normal_mode {
  struct hikari_mode mode;
};

void
hikari_normal_mode_init(struct hikari_normal_mode *normal_mode);

#endif
