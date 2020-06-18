#if !defined(HIKARIINDICATOR_FRAME_H)
#define HIKARIINDICATOR_FRAME_H

#include <wlr/types/wlr_box.h>

struct hikari_view;
struct hikari_renderer;

struct hikari_indicator_frame {
  struct wlr_box top;
  struct wlr_box bottom;
  struct wlr_box left;
  struct wlr_box right;
};

void
hikari_indicator_frame_refresh_geometry(
    struct hikari_indicator_frame *indicator_frame, struct hikari_view *view);

void
hikari_indicator_frame_render(struct hikari_indicator_frame *indicator_frame,
    float color[static 4],
    struct hikari_renderer *renderer);

#endif
