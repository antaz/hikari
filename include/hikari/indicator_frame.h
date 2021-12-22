#if !defined(HIKARIINDICATOR_FRAME_H)
#define HIKARIINDICATOR_FRAME_H

#include <wlr/util/box.h>

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

#endif
