#if !defined(HIKARI_MARK_SELECT_MODE_H)
#define HIKARI_MARK_SELECT_MODE_H

#include <stdbool.h>

#include <hikari/mode.h>

struct hikari_mark_select_mode {
  struct hikari_mode mode;

  bool switch_workspace;
};

void
hikari_mark_select_mode_init(struct hikari_mark_select_mode *mark_select_mode);

void
hikari_mark_select_mode_enter(bool swith_workspace);

#endif
