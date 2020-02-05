#if !defined(HIKARI_EXEC_SELECT_MODE_H)
#define HIKARI_EXEC_SELECT_MODE_H

#include <stdbool.h>

#include <hikari/mode.h>

struct hikari_exec_select_mode {
  struct hikari_mode mode;
};

void
hikari_exec_select_mode_init(struct hikari_exec_select_mode *exec_select_mode);

#endif
