#if !defined(HIKARI_BORDER_H)
#define HIKARI_BORDER_H

#include <wlr/util/box.h>

#include <hikari/output.h>

struct hikari_renderer;

enum hikari_border_state {
  HIKARI_BORDER_NONE,
  HIKARI_BORDER_INACTIVE,
  HIKARI_BORDER_ACTIVE
};

struct hikari_border {
  enum hikari_border_state state;

  struct wlr_box geometry;
  struct wlr_box top;
  struct wlr_box bottom;
  struct wlr_box left;
  struct wlr_box right;
};

static inline struct wlr_box *
hikari_border_geometry(struct hikari_border *border)
{
  return &border->geometry;
}

void
hikari_border_refresh_geometry(
    struct hikari_border *border, struct wlr_box *geometry);

#endif
