#if !defined(HIKARI_LOCK_MODE_H)
#define HIKARI_LOCK_MODE_H

#include <stdbool.h>

#include <wayland-util.h>

#include <hikari/lock_indicator.h>
#include <hikari/mode.h>

struct hikari_lock_mode {
  struct hikari_mode mode;
  struct wl_event_source *disable_outputs;
  struct hikari_lock_indicator *lock_indicator;

  bool outputs_disabled;
};

void
hikari_lock_mode_init(struct hikari_lock_mode *lock_mode);

void
hikari_lock_mode_fini(struct hikari_lock_mode *lock_mode);

void
hikari_lock_mode_enter(void);

static inline bool
hikari_lock_mode_are_outputs_disabled(struct hikari_lock_mode *lock_mode)
{
  return lock_mode->outputs_disabled;
}

#endif
