#include <hikari/border.h>

#include <assert.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/output.h>
#include <hikari/render_data.h>
#include <hikari/view_interface.h>

static void
rect_render(float color[static 4],
    struct wlr_box *box,
    struct hikari_render_data *render_data)
{
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

  float matrix[9];
  wlr_matrix_project_box(matrix,
      box,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      render_data->output->transform_matrix);

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; i++) {
    hikari_output_scissor_render(render_data->output, renderer, &rects[i]);
    wlr_render_quad_with_matrix(renderer, color, matrix);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
}

void
hikari_border_render(
    struct hikari_border *border, struct hikari_render_data *render_data)
{
  if (border->state == HIKARI_BORDER_NONE) {
    return;
  }

  struct wlr_box *geometry = &border->geometry;

  pixman_region32_t damage;
  pixman_region32_init(&damage);
  pixman_region32_union_rect(&damage,
      &damage,
      geometry->x,
      geometry->y,
      geometry->width,
      geometry->height);
  pixman_region32_intersect(&damage, &damage, render_data->damage);

  bool damaged = pixman_region32_not_empty(&damage);
  if (!damaged) {
    goto buffer_damage_finish;
  }

  float *color;
  switch (border->state) {
    case HIKARI_BORDER_INACTIVE:
      color = hikari_configuration.border_inactive;
      break;

    case HIKARI_BORDER_ACTIVE:
      color = hikari_configuration.border_active;
      break;

    default:
      goto buffer_damage_finish;
  }

  struct wlr_renderer *renderer = render_data->renderer;
  assert(renderer);

  float matrix[9];
  wlr_matrix_project_box(matrix,
      geometry,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      render_data->output->transform_matrix);

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; i++) {
    hikari_output_scissor_render(render_data->output, renderer, &rects[i]);
    rect_render(color, &border->top, render_data);
    rect_render(color, &border->bottom, render_data);
    rect_render(color, &border->left, render_data);
    rect_render(color, &border->right, render_data);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
}

void
hikari_border_refresh_geometry(
    struct hikari_border *border, struct wlr_box *geometry)
{
  if (border->state == HIKARI_BORDER_NONE) {
    border->geometry = *geometry;
    return;
  } else {
    border->geometry.x = geometry->x - hikari_configuration.border;
    border->geometry.y = geometry->y - hikari_configuration.border;
    border->geometry.width = geometry->width + hikari_configuration.border * 2;
    border->geometry.height =
        geometry->height + hikari_configuration.border * 2;
  }

  border->top.x = border->geometry.x;
  border->top.y = border->geometry.y;
  border->top.width = border->geometry.width;
  border->top.height = hikari_configuration.border;

  border->bottom.x = border->geometry.x;
  border->bottom.y = border->geometry.y + border->geometry.height -
                     hikari_configuration.border;
  border->bottom.width = border->geometry.width;
  border->bottom.height = hikari_configuration.border;

  border->left.x = border->geometry.x;
  border->left.y = border->geometry.y;
  border->left.width = hikari_configuration.border;
  border->left.height = border->geometry.height;

  border->right.x =
      border->geometry.x + border->geometry.width - hikari_configuration.border;
  border->right.y = border->geometry.y;
  border->right.width = hikari_configuration.border;
  border->right.height = border->geometry.height;
}
