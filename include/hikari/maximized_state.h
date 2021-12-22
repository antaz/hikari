#if !defined(HIKARIMAXIMIZED_STATE_H)
#define HIKARIMAXIMIZED_STATE_H

#include <stdlib.h>

#include <wlr/util/box.h>

#include <hikari/memory.h>

struct hikari_view;

enum hikari_maximization {
  HIKARI_MAXIMIZATION_FULLY_MAXIMIZED,
  HIKARI_MAXIMIZATION_VERTICALLY_MAXIMIZED,
  HIKARI_MAXIMIZATION_HORIZONTALLY_MAXIMIZED
};

struct hikari_maximized_state {
  enum hikari_maximization maximization;
  struct wlr_box geometry;
};

struct hikari_maximized_state *
hikari_maximized_state_full(struct hikari_view *view, int width, int height);

static inline struct hikari_maximized_state *
hikari_maximized_state_alloc(void)
{
  return hikari_malloc(sizeof(struct hikari_maximized_state));
}

static inline void
hikari_maximized_state_destroy(struct hikari_maximized_state *maximized_state)
{
  hikari_free(maximized_state);
}

#endif
