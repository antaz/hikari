#include <hikari/indicator_frame.h>

#include <assert.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>

#include <hikari/border.h>
#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/output.h>
#include <hikari/renderer.h>
#include <hikari/view.h>

void
hikari_indicator_frame_refresh_geometry(
    struct hikari_indicator_frame *indicator_frame, struct hikari_view *view)
{
  struct wlr_box *geometry;

  if (view->border.state == HIKARI_BORDER_NONE) {
    geometry = hikari_view_geometry(view);
  } else {
    geometry = hikari_view_border_geometry(view);
  }

  int border = hikari_configuration->border;

  indicator_frame->top.x = geometry->x;
  indicator_frame->top.y = geometry->y;
  indicator_frame->top.width = geometry->width;
  indicator_frame->top.height = border;

  indicator_frame->bottom.x = geometry->x;
  indicator_frame->bottom.y = geometry->y + geometry->height - border;
  indicator_frame->bottom.width = geometry->width;
  indicator_frame->bottom.height = border;

  indicator_frame->left.x = geometry->x;
  indicator_frame->left.y = geometry->y;
  indicator_frame->left.width = border;
  indicator_frame->left.height = geometry->height;

  indicator_frame->right.x = geometry->x + geometry->width - border;
  indicator_frame->right.y = geometry->y;
  indicator_frame->right.width = border;
  indicator_frame->right.height = geometry->height;
}
