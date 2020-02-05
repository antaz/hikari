#if !defined(HIKARI_RENDER_DATA_H)
#define HIKARI_RENDER_DATA_H

#include <wlr/types/wlr_box.h>

struct wlr_output *output;
struct wlr_renderer *renderer;
struct timespec *when;

struct hikari_render_data {
  struct wlr_output *output;
  struct wlr_renderer *renderer;
  pixman_region32_t *damage;
  struct wlr_box *geometry;
  struct timespec *when;
};

#endif
