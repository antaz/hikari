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
  struct wlr_box *top_bottom_geometry;
  struct wlr_box *left_right_geometry;

  if (view->border.state == HIKARI_BORDER_NONE) {
    top_bottom_geometry = hikari_view_geometry(view);
    left_right_geometry = top_bottom_geometry;
  } else if (view->maximized_state == NULL) {
    top_bottom_geometry = hikari_view_border_geometry(view);
    left_right_geometry = top_bottom_geometry;
  } else {
    switch (view->maximized_state->maximization) {
      case HIKARI_MAXIMIZATION_VERTICALLY_MAXIMIZED:
        top_bottom_geometry = hikari_view_geometry(view);
        left_right_geometry = hikari_view_border_geometry(view);
        break;

      case HIKARI_MAXIMIZATION_HORIZONTALLY_MAXIMIZED:
        top_bottom_geometry = hikari_view_border_geometry(view);
        left_right_geometry = hikari_view_geometry(view);
        break;

      case HIKARI_MAXIMIZATION_FULLY_MAXIMIZED:
        top_bottom_geometry = hikari_view_geometry(view);
        left_right_geometry = top_bottom_geometry;
        break;
    }
  }

  int border = hikari_configuration->border;

  indicator_frame->top.x = top_bottom_geometry->x;
  indicator_frame->top.y = top_bottom_geometry->y;
  indicator_frame->top.width = top_bottom_geometry->width;
  indicator_frame->top.height = border;

  indicator_frame->bottom.x = top_bottom_geometry->x;
  indicator_frame->bottom.y =
      top_bottom_geometry->y + top_bottom_geometry->height - border;
  indicator_frame->bottom.width = top_bottom_geometry->width;
  indicator_frame->bottom.height = border;

  indicator_frame->left.x = left_right_geometry->x;
  indicator_frame->left.y = left_right_geometry->y;
  indicator_frame->left.width = border;
  indicator_frame->left.height = left_right_geometry->height;

  indicator_frame->right.x =
      left_right_geometry->x + left_right_geometry->width - border;
  indicator_frame->right.y = left_right_geometry->y;
  indicator_frame->right.width = border;
  indicator_frame->right.height = left_right_geometry->height;
}
