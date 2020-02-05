#if !defined(HIKARI_MOVE_MODE_H)
#define HIKARI_MOVE_MODE_H

#include <hikari/mode.h>

struct hikari_keybinding;

struct hikari_move_mode {
  struct hikari_mode mode;
};

void
hikari_move_mode_init(struct hikari_move_mode *move_mode);

#endif
