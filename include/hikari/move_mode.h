#if !defined(HIKARI_MOVE_MODE_H)
#define HIKARI_MOVE_MODE_H

#include <hikari/mode.h>

struct hikari_binding;
struct hikari_view;

struct hikari_move_mode {
  struct hikari_mode mode;
};

void
hikari_move_mode_init(struct hikari_move_mode *move_mode);

void
hikari_move_mode_enter(struct hikari_view *view);

#endif
