#if !defined(HIKARI_RENDER_DATA_H)
#define HIKARI_RENDER_DATA_H

#include <time.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_output.h>

struct hikari_render_data {
  struct wlr_output *output;
  struct wlr_renderer *renderer;
  pixman_region32_t *damage;
  struct wlr_box *geometry;
  struct timespec *when;
};

#endif
