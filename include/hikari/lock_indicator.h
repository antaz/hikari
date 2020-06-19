#if !defined(HIKARI_LOCK_INDICATOR_H)
#define HIKARI_LOCK_INDICATOR_H

#include <wayland-util.h>

#include <wlr/render/wlr_texture.h>

struct hikari_output;

struct hikari_lock_indicator {
  struct wlr_texture *wait;
  struct wlr_texture *type;
  struct wlr_texture *verify;
  struct wlr_texture *deny;

  struct wlr_texture *current;

  struct wl_event_source *reset_state;
};

void
hikari_lock_indicator_init(struct hikari_lock_indicator *lock_indicator);

void
hikari_lock_indicator_fini(struct hikari_lock_indicator *lock_indicator);

void
hikari_lock_indicator_set_wait(struct hikari_lock_indicator *lock_indicator);

void
hikari_lock_indicator_set_type(struct hikari_lock_indicator *lock_indicator);

void
hikari_lock_indicator_set_verify(struct hikari_lock_indicator *lock_indicator);

void
hikari_lock_indicator_set_deny(struct hikari_lock_indicator *lock_indicator);

void
hikari_lock_indicator_clear(struct hikari_lock_indicator *lock_indicator);

void
hikari_lock_indicator_damage(struct hikari_lock_indicator *lock_indicator);

#endif
