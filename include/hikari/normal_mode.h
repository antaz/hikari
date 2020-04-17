#if !defined(HIKARI_NORMAL_MODE_H)
#define HIKARI_NORMAL_MODE_H

#include <hikari/mode.h>

struct hikari_event_action;

struct hikari_normal_mode {
  struct hikari_mode mode;
  struct hikari_event_action *pending_action;
};

void
hikari_normal_mode_init(struct hikari_normal_mode *normal_mode);

void
hikari_normal_mode_enter(void);

#endif
