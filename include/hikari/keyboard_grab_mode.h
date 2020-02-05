#if !defined(HIKARI_KEYBOARD_GRAB_MODE_H)
#define HIKARI_KEYBOARD_GRAB_MODE_H

#include <hikari/mode.h>

struct hikari_keyboard_grab_mode {
  struct hikari_mode mode;
};

void
hikari_keyboard_grab_mode_init(
    struct hikari_keyboard_grab_mode *keyboard_grab_mode);

#endif
