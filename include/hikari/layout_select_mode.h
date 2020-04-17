#if !defined(HIKARI_LAYOUT_SELECT_MODE_H)
#define HIKARI_LAYOUT_SELECT_MODE_H

#include <stdbool.h>

#include <hikari/mode.h>

struct hikari_layout_select_mode {
  struct hikari_mode mode;
};

void
hikari_layout_select_mode_init(
    struct hikari_layout_select_mode *layout_select_mode);
void
hikari_layout_select_mode_enter(void);

#endif
