#if !defined(HIKARI_INPUT_GRAB_MODE_H)
#define HIKARI_INPUT_GRAB_MODE_H

#include <hikari/mode.h>

struct hikari_input_grab_mode {
  struct hikari_mode mode;
};

void
hikari_input_grab_mode_init(struct hikari_input_grab_mode *input_grab_mode);

#endif
