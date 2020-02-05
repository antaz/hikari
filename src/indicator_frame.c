#include <hikari/indicator_frame.h>

#include <assert.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>

#include <hikari/border.h>
#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/output.h>
#include <hikari/render_data.h>
#include <hikari/view.h>

void
hikari_indicator_frame_render(struct hikari_indicator_frame *indicator_frame,
    float color[static 4],
    struct hikari_render_data *render_data)
{
  struct wlr_box *box = render_data->geometry;

  pixman_region32_t damage;
  pixman_region32_init(&damage);
  pixman_region32_union_rect(
      &damage, &damage, box->x, box->y, box->width, box->height);

  pixman_region32_intersect(&damage, &damage, render_data->damage);
  bool damaged = pixman_region32_not_empty(&damage);
  if (!damaged) {
    goto buffer_damage_finish;
  }

  struct wlr_renderer *renderer = render_data->renderer;
  assert(renderer);

  float top_matrix[9];
  float bottom_matrix[9];
  float left_matrix[9];
  float right_matrix[9];

  wlr_matrix_project_box(top_matrix,
      &indicator_frame->top,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      render_data->output->transform_matrix);

  wlr_matrix_project_box(bottom_matrix,
      &indicator_frame->bottom,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      render_data->output->transform_matrix);

  wlr_matrix_project_box(left_matrix,
      &indicator_frame->left,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      render_data->output->transform_matrix);

  wlr_matrix_project_box(right_matrix,
      &indicator_frame->right,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      render_data->output->transform_matrix);

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; i++) {
    hikari_output_scissor_render(render_data->output, renderer, &rects[i]);
    wlr_render_quad_with_matrix(renderer, color, top_matrix);
    wlr_render_quad_with_matrix(renderer, color, bottom_matrix);
    wlr_render_quad_with_matrix(renderer, color, left_matrix);
    wlr_render_quad_with_matrix(renderer, color, right_matrix);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
}

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

  indicator_frame->top.x = geometry->x;
  indicator_frame->top.y = geometry->y;
  indicator_frame->top.width = geometry->width;
  indicator_frame->top.height = hikari_configuration.border;

  indicator_frame->bottom.x = geometry->x;
  indicator_frame->bottom.y =
      geometry->y + geometry->height - hikari_configuration.border;
  indicator_frame->bottom.width = geometry->width;
  indicator_frame->bottom.height = hikari_configuration.border;

  indicator_frame->left.x = geometry->x;
  indicator_frame->left.y = geometry->y;
  indicator_frame->left.width = hikari_configuration.border;
  indicator_frame->left.height = geometry->height;

  indicator_frame->right.x =
      geometry->x + geometry->width - hikari_configuration.border;
  indicator_frame->right.y = geometry->y;
  indicator_frame->right.width = hikari_configuration.border;
  indicator_frame->right.height = geometry->height;
}
