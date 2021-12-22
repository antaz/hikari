#include <hikari/renderer.h>

#include <assert.h>

#include <hikari/color.h>
#include <hikari/geometry.h>
#include <hikari/output.h>
#include <hikari/renderer.h>
#include <hikari/view.h>

#ifdef HAVE_XWAYLAND
#include <hikari/xwayland_unmanaged_view.h>
#include <hikari/xwayland_view.h>
#endif

#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/util/region.h>

#ifdef HAVE_XWAYLAND
#include <wlr/xwayland.h>
#endif

static inline void
renderer_scissor(struct wlr_output *wlr_output,
    struct wlr_renderer *renderer,
    pixman_box32_t *rect)
{
  assert(wlr_output != NULL);

  struct wlr_box box = { .x = rect->x1,
    .y = rect->y1,
    .width = rect->x2 - rect->x1,
    .height = rect->y2 - rect->y1 };

  wlr_renderer_scissor(renderer, &box);
}

static inline void
rect_render(float color[static 4],
    struct wlr_box *box,
    struct hikari_renderer *renderer)
{
  pixman_region32_t damage;
  pixman_region32_init(&damage);
  pixman_region32_union_rect(
      &damage, &damage, box->x, box->y, box->width, box->height);

  pixman_region32_intersect(&damage, &damage, renderer->damage);
  bool damaged = pixman_region32_not_empty(&damage);
  if (!damaged) {
    goto buffer_damage_finish;
  }

  struct wlr_renderer *wlr_renderer = renderer->wlr_renderer;
  struct wlr_output *wlr_output = renderer->wlr_output;
  assert(renderer);

  float matrix[9];
  wlr_matrix_project_box(
      matrix, box, WL_OUTPUT_TRANSFORM_NORMAL, 0, wlr_output->transform_matrix);

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; i++) {
    renderer_scissor(wlr_output, wlr_renderer, &rects[i]);
    wlr_render_quad_with_matrix(wlr_renderer, color, matrix);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
}

static inline void
render_border(struct hikari_border *border, struct hikari_renderer *renderer)
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
  pixman_region32_intersect(&damage, &damage, renderer->damage);

  bool damaged = pixman_region32_not_empty(&damage);
  if (!damaged) {
    goto buffer_damage_finish;
  }

  float *color;
  switch (border->state) {
    case HIKARI_BORDER_INACTIVE:
      color = hikari_configuration->border_inactive;
      break;

    case HIKARI_BORDER_ACTIVE:
      color = hikari_configuration->border_active;
      break;

    default:
      goto buffer_damage_finish;
  }

  struct wlr_renderer *wlr_renderer = renderer->wlr_renderer;
  struct wlr_output *wlr_output = renderer->wlr_output;
  assert(renderer);

  float matrix[9];
  wlr_matrix_project_box(matrix,
      geometry,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      wlr_output->transform_matrix);

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; i++) {
    renderer_scissor(wlr_output, wlr_renderer, &rects[i]);
    rect_render(color, &border->top, renderer);
    rect_render(color, &border->bottom, renderer);
    rect_render(color, &border->left, renderer);
    rect_render(color, &border->right, renderer);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
}

static void
render_indicator_bar(struct hikari_indicator_bar *indicator_bar,
    struct hikari_renderer *renderer)
{
  if (indicator_bar->texture == NULL) {
    return;
  }

  struct wlr_box *geometry = renderer->geometry;
  struct wlr_renderer *wlr_renderer = renderer->wlr_renderer;
  struct wlr_output *wlr_output = renderer->wlr_output;

  float matrix[9];

  geometry->width = indicator_bar->width;
  geometry->height = hikari_configuration->font.height;

  wlr_renderer_scissor(wlr_renderer, geometry);
  wlr_matrix_project_box(matrix, geometry, 0, 0, wlr_output->transform_matrix);

  wlr_render_texture_with_matrix(
      wlr_renderer, indicator_bar->texture, matrix, 1);
}

static inline void
render_indicator(
    struct hikari_indicator *indicator, struct hikari_renderer *renderer)
{
  struct wlr_box *border_geometry = renderer->geometry;
  struct wlr_box geometry = *border_geometry;

  renderer->geometry = &geometry;

  geometry.x += 5;

  struct hikari_indicator_bar *title_bar = &indicator->title;
  geometry.y += 5;
  render_indicator_bar(title_bar, renderer);

  int bar_height = hikari_configuration->font.height;

  struct hikari_indicator_bar *sheet_bar = &indicator->sheet;
  geometry.y += bar_height + 5;
  render_indicator_bar(sheet_bar, renderer);

  struct hikari_indicator_bar *group_bar = &indicator->group;
  geometry.y += bar_height + 5;
  render_indicator_bar(group_bar, renderer);

  struct hikari_indicator_bar *mark_bar = &indicator->mark;
  geometry.y += bar_height + 5;
  render_indicator_bar(mark_bar, renderer);

  renderer->geometry = border_geometry;
}

static inline void
render_indicator_frame(struct hikari_indicator_frame *indicator_frame,
    float color[static 4],
    struct hikari_renderer *renderer)
{
  struct wlr_box *box = renderer->geometry;

  pixman_region32_t damage;
  pixman_region32_init(&damage);
  pixman_region32_union_rect(
      &damage, &damage, box->x, box->y, box->width, box->height);

  pixman_region32_intersect(&damage, &damage, renderer->damage);
  bool damaged = pixman_region32_not_empty(&damage);
  if (!damaged) {
    goto buffer_damage_finish;
  }

  struct wlr_renderer *wlr_renderer = renderer->wlr_renderer;
  struct wlr_output *wlr_output = renderer->wlr_output;

  float top_matrix[9];
  float bottom_matrix[9];
  float left_matrix[9];
  float right_matrix[9];

  wlr_matrix_project_box(top_matrix,
      &indicator_frame->top,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      wlr_output->transform_matrix);

  wlr_matrix_project_box(bottom_matrix,
      &indicator_frame->bottom,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      wlr_output->transform_matrix);

  wlr_matrix_project_box(left_matrix,
      &indicator_frame->left,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      wlr_output->transform_matrix);

  wlr_matrix_project_box(right_matrix,
      &indicator_frame->right,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      wlr_output->transform_matrix);

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; i++) {
    renderer_scissor(wlr_output, wlr_renderer, &rects[i]);
    wlr_render_quad_with_matrix(wlr_renderer, color, top_matrix);
    wlr_render_quad_with_matrix(wlr_renderer, color, bottom_matrix);
    wlr_render_quad_with_matrix(wlr_renderer, color, left_matrix);
    wlr_render_quad_with_matrix(wlr_renderer, color, right_matrix);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
}

static inline void
clear_output(struct hikari_renderer *renderer)
{
  float *clear_color = hikari_configuration->clear;
  struct wlr_renderer *wlr_renderer = renderer->wlr_renderer;
  struct wlr_output *wlr_output = renderer->wlr_output;
  pixman_region32_t *damage = renderer->damage;

#ifndef NDEBUG
  if (hikari_server.track_damage) {
    float damage_color[4];
    hikari_color_convert(damage_color, 0x000000);
    wlr_renderer_clear(wlr_renderer, damage_color);
  }
#endif

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(damage, &nrects);
  for (int i = 0; i < nrects; ++i) {
    renderer_scissor(wlr_output, wlr_renderer, &rects[i]);
    wlr_renderer_clear(wlr_renderer, clear_color);
  }
}

static inline void
renderer_end(struct hikari_output *output, struct hikari_renderer *renderer)
{
  struct wlr_renderer *wlr_renderer = renderer->wlr_renderer;
  struct wlr_output *wlr_output = renderer->wlr_output;

  wlr_renderer_scissor(wlr_renderer, NULL);
  wlr_output_render_software_cursors(wlr_output, NULL);
  wlr_renderer_end(wlr_renderer);

  int width, height;
  wlr_output_transformed_resolution(wlr_output, &width, &height);

  pixman_region32_t frame_damage;
  pixman_region32_init(&frame_damage);

  enum wl_output_transform transform =
      wlr_output_transform_invert(wlr_output->transform);
  wlr_region_transform(
      &frame_damage, &output->damage->current, transform, width, height);

  wlr_output_set_damage(wlr_output, &frame_damage);
  pixman_region32_fini(&frame_damage);

  wlr_output_commit(wlr_output);
}

static inline void
render_texture(struct wlr_texture *texture,
    struct wlr_output *output,
    pixman_region32_t *damage,
    struct wlr_renderer *renderer,
    const float matrix[static 9],
    struct wlr_box *box,
    float alpha)
{
  pixman_region32_t local_damage;
  pixman_region32_init(&local_damage);
  pixman_region32_union_rect(
      &local_damage, &local_damage, box->x, box->y, box->width, box->height);

  pixman_region32_intersect(&local_damage, &local_damage, damage);

  bool damaged = pixman_region32_not_empty(&local_damage);
  if (!damaged) {
    goto damage_finish;
  }

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&local_damage, &nrects);
  for (int i = 0; i < nrects; ++i) {
    renderer_scissor(output, renderer, &rects[i]);
    wlr_render_texture_with_matrix(renderer, texture, matrix, alpha);
  }

damage_finish:
  pixman_region32_fini(&local_damage);
}

static void
render_surface(struct wlr_surface *surface, int sx, int sy, void *data)
{
  assert(surface != NULL);

  struct wlr_texture *texture = wlr_surface_get_texture(surface);

  if (texture == NULL) {
    return;
  }

  struct hikari_renderer *renderer = data;
  struct wlr_box *geometry = renderer->geometry;
  struct wlr_output *wlr_output = renderer->wlr_output;
  struct wlr_renderer *wlr_renderer = renderer->wlr_renderer;

  double ox = geometry->x + sx;
  double oy = geometry->y + sy;

  struct wlr_box box = { .x = ox * wlr_output->scale,
    .y = oy * wlr_output->scale,
    .width = surface->current.width * wlr_output->scale,
    .height = surface->current.height * wlr_output->scale };

  float matrix[9];
  enum wl_output_transform transform =
      wlr_output_transform_invert(surface->current.transform);

  wlr_matrix_project_box(
      matrix, &box, transform, 0, wlr_output->transform_matrix);

  render_texture(
      texture, wlr_output, renderer->damage, wlr_renderer, matrix, &box, 1);
}

static inline void
render_background(struct hikari_renderer *renderer, float alpha)
{
  struct hikari_output *output = renderer->wlr_output->data;

  if (output->background == NULL) {
    return;
  }

  float matrix[9];
  struct wlr_output *wlr_output = output->wlr_output;
  struct wlr_renderer *wlr_renderer = renderer->wlr_renderer;

  struct wlr_box geometry = { .x = 0, .y = 0 };
  wlr_output_transformed_resolution(
      wlr_output, &geometry.width, &geometry.height);

  wlr_matrix_project_box(matrix, &geometry, 0, 0, wlr_output->transform_matrix);

  render_texture(output->background,
      wlr_output,
      renderer->damage,
      wlr_renderer,
      matrix,
      &geometry,
      alpha);
}

#ifdef HAVE_LAYERSHELL
static inline void
render_layer(struct wl_list *layers, struct hikari_renderer *renderer)
{
  struct hikari_layer *layer;
  wl_list_for_each (layer, layers, layer_surfaces) {
    renderer->geometry = &layer->geometry;
    wlr_layer_surface_v1_for_each_surface(
        layer->surface, render_surface, renderer);
  }
}
#endif

static inline void
render_view(struct hikari_renderer *renderer, struct hikari_view *view)
{
  renderer->geometry = hikari_view_border_geometry(view);

  if (hikari_view_wants_border(view)) {
    render_border(&view->border, renderer);
  }

  renderer->geometry = hikari_view_geometry(view);

  hikari_node_for_each_surface(
      (struct hikari_node *)view, render_surface, renderer);
}

#ifdef HAVE_XWAYLAND
static inline void
render_unmanaged_views(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view;
  wl_list_for_each_reverse (xwayland_unmanaged_view,
      &output->unmanaged_xwayland_views,
      unmanaged_output_views) {

    renderer->geometry = &xwayland_unmanaged_view->geometry;

    wlr_surface_for_each_surface(
        xwayland_unmanaged_view->surface->surface, render_surface, renderer);
  }
}
#endif

static inline void
render_workspace(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

#ifdef HAVE_LAYERSHELL
  render_layer(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], renderer);
  render_layer(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], renderer);
#endif

  struct hikari_view *view;
  wl_list_for_each_reverse (view, &output->workspace->views, workspace_views) {
    render_view(renderer, view);
  }

#ifdef HAVE_LAYERSHELL
  render_layer(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], renderer);
#endif

#ifdef HAVE_XWAYLAND
  render_unmanaged_views(renderer);
#endif
}

#ifdef HAVE_LAYERSHELL
static inline void
render_overlay(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;
  render_layer(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], renderer);
}
#endif

static inline void
render_output(struct hikari_output *output, pixman_region32_t *damage)
{
  struct wlr_output *wlr_output = output->wlr_output;
  struct wlr_renderer *wlr_renderer = wlr_output->renderer;

  struct hikari_renderer renderer = {
    .wlr_output = wlr_output, .wlr_renderer = wlr_renderer, .damage = damage
  };

  wlr_renderer_begin(wlr_renderer, wlr_output->width, wlr_output->height);

  if (pixman_region32_not_empty(damage)) {
    clear_output(&renderer);

    hikari_server.mode->render(&renderer);
  }

  renderer_end(output, &renderer);
}

#ifdef HAVE_LAYERSHELL
static inline void
layer_for_each(struct wl_list *layers,
    void (*func)(struct wlr_surface *, int, int, void *),
    void *data)
{
  struct hikari_layer *layer;
  wl_list_for_each (layer, layers, layer_surfaces) {
    wlr_layer_surface_v1_for_each_surface(layer->surface, func, data);
  }
}
#endif

static void
send_frame_done(struct wlr_surface *surface, int sx, int sy, void *data)
{
  assert(surface != NULL);

  struct timespec *now = data;
  wlr_surface_send_frame_done(surface, now);
}

static inline void
frame_done(struct hikari_output *output)
{
  struct hikari_view *view;
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  wl_list_for_each_reverse (view, &output->views, output_views) {
    hikari_node_for_each_surface(
        (struct hikari_node *)view, send_frame_done, &now);
  }

#ifdef HAVE_XWAYLAND
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view;
  wl_list_for_each_reverse (xwayland_unmanaged_view,
      &output->unmanaged_xwayland_views,
      unmanaged_output_views) {
    wlr_surface_for_each_surface(
        xwayland_unmanaged_view->surface->surface, send_frame_done, &now);
  }
#endif

#ifdef HAVE_LAYERSHELL
  for (int i = 0; i < 4; i++) {
    layer_for_each(&output->layers[i], send_frame_done, &now);
  }
#endif
}

void
hikari_renderer_damage_frame_handler(struct wl_listener *listener, void *data)
{
  struct hikari_output *output =
      wl_container_of(listener, output, damage_frame);

  pixman_region32_t buffer_damage;
  pixman_region32_init(&buffer_damage);

  bool needs_frame;
  if (!wlr_output_damage_attach_render(
          output->damage, &needs_frame, &buffer_damage)) {
    goto render_done;
  }

  if (!needs_frame) {
    wlr_output_rollback(output->wlr_output);
    goto render_done;
  }

  render_output(output, &buffer_damage);

render_done:
  pixman_region32_fini(&buffer_damage);

  frame_done(output);
}

static inline void
render_public_views(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

  struct hikari_view *view;
  wl_list_for_each_reverse (view, &output->views, output_views) {
    if (hikari_view_is_public(view) && !hikari_view_is_hidden(view)) {
      renderer->geometry = hikari_view_border_geometry(view);

      if (hikari_view_wants_border(view)) {
        render_border(&view->border, renderer);
      }

      renderer->geometry = hikari_view_geometry(view);

      hikari_node_for_each_surface(
          (struct hikari_node *)view, render_surface, renderer);
    }
  }
}

static inline void
render_normal_mode_indication(
    struct hikari_renderer *renderer, struct hikari_view *focus_view)
{
  struct hikari_output *output = renderer->wlr_output->data;
  struct hikari_group *group = focus_view->group;
  struct hikari_view *first = hikari_group_first_view(group);
  float *indicator_first = hikari_configuration->indicator_first;
  float *indicator_grouped = hikari_configuration->indicator_grouped;

  struct hikari_view *view;
  wl_list_for_each_reverse (view, &group->visible_views, visible_group_views) {
    if (view != focus_view && view->output == output) {
      renderer->geometry = hikari_view_border_geometry(view);

      if (first == view) {
        render_indicator_frame(
            &view->indicator_frame, indicator_first, renderer);
      } else {
        render_indicator_frame(
            &view->indicator_frame, indicator_grouped, renderer);
      }
    }
  }

  if (focus_view->output == output) {
    renderer->geometry = hikari_view_border_geometry(focus_view);

    render_indicator_frame(&focus_view->indicator_frame,
        hikari_configuration->indicator_selected,
        renderer);

    render_indicator(&hikari_server.indicator, renderer);
  }
}

static inline void
render_cycling_workspace(
    struct hikari_renderer *renderer, struct hikari_view *focus_view)
{
  struct hikari_output *output = renderer->wlr_output->data;

#ifdef HAVE_LAYERSHELL
  render_layer(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], renderer);
  render_layer(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], renderer);
#endif

  struct hikari_view *view;
  wl_list_for_each_reverse (view, &output->workspace->views, workspace_views) {
    if (view != focus_view) {
      render_view(renderer, view);
    }
  }

  if (focus_view->output == output) {
    render_view(renderer, focus_view);
  }

#ifdef HAVE_LAYERSHELL
  render_layer(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], renderer);
#endif

#ifdef HAVE_XWAYLAND
  render_unmanaged_views(renderer);
#endif
}

void
hikari_renderer_normal_mode(struct hikari_renderer *renderer)
{
  render_background(renderer, 1);

  if (!hikari_server_is_indicating()) {
    render_workspace(renderer);
  } else {
    struct hikari_view *focus_view = hikari_server.workspace->focus_view;

    if (focus_view != NULL) {
      if (hikari_server_is_cycling()) {
        render_cycling_workspace(renderer, focus_view);
      } else {
        render_workspace(renderer);
      }
      render_normal_mode_indication(renderer, focus_view);
    } else {
      render_workspace(renderer);
    }
  }

#ifdef HAVE_LAYERSHELL
  render_overlay(renderer);
#endif
}

void
hikari_renderer_group_assign_mode(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

  render_background(renderer, 1);
  render_workspace(renderer);

  struct hikari_group_assign_mode *mode = &hikari_server.group_assign_mode;

  assert(mode == (struct hikari_group_assign_mode *)hikari_server.mode);

  struct hikari_group *group = mode->group;
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  assert(focus_view != NULL);

  if (group != NULL) {
    struct hikari_view *first = hikari_group_first_view(group);
    float *indicator_first = hikari_configuration->indicator_first;
    float *indicator_grouped = hikari_configuration->indicator_grouped;

    struct hikari_view *view;
    wl_list_for_each_reverse (
        view, &group->visible_views, visible_group_views) {
      if (view->output == output && view != focus_view) {
        renderer->geometry = hikari_view_border_geometry(view);

        if (first == view) {
          render_indicator_frame(
              &view->indicator_frame, indicator_first, renderer);
        } else {
          render_indicator_frame(
              &view->indicator_frame, indicator_grouped, renderer);
        }
      }
    }
  }

  if (focus_view->output == output) {
    renderer->geometry = hikari_view_border_geometry(focus_view);

    render_indicator_frame(&focus_view->indicator_frame,
        hikari_configuration->indicator_selected,
        renderer);

    render_indicator(&hikari_server.indicator, renderer);
  }

#ifdef HAVE_LAYERSHELL
  render_overlay(renderer);
#endif
}

void
hikari_renderer_input_grab_mode(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

  render_background(renderer, 1);
  render_workspace(renderer);

  assert(hikari_server.workspace->focus_view != NULL);

  struct hikari_view *view = hikari_server.workspace->focus_view;

  if (view->output == output) {
    renderer->geometry = hikari_view_border_geometry(view);
    render_indicator_frame(&view->indicator_frame,
        hikari_configuration->indicator_insert,
        renderer);
  }

#ifdef HAVE_LAYERSHELL
  render_overlay(renderer);
#endif
}

static inline void
get_lock_indicator_geometry(
    struct hikari_output *output, struct wlr_box *geometry)
{
  const int size = 100;

  geometry->width = size;
  geometry->height = size;

  struct wlr_box output_geometry = { .x = 0,
    .y = 0,
    .width = output->geometry.width,
    .height = output->geometry.height };

  hikari_geometry_position_center(
      geometry, &output_geometry, &geometry->x, &geometry->y);
}

static inline void
render_lock_indicator(struct hikari_renderer *renderer,
    struct hikari_lock_indicator *lock_indicator)
{
  assert(lock_indicator != NULL);

  struct wlr_texture *texture = lock_indicator->current;

  if (texture == NULL) {
    return;
  }

  float matrix[9];
  struct wlr_renderer *wlr_renderer = renderer->wlr_renderer;
  struct wlr_output *wlr_output = renderer->wlr_output;

  struct wlr_box geometry;
  get_lock_indicator_geometry(wlr_output->data, &geometry);
  wlr_renderer_scissor(wlr_renderer, &geometry);
  wlr_matrix_project_box(matrix, &geometry, 0, 0, wlr_output->transform_matrix);

  wlr_render_texture_with_matrix(wlr_renderer, texture, matrix, 1);
}

void
hikari_renderer_lock_mode(struct hikari_renderer *renderer)
{
  struct hikari_lock_mode *mode = &hikari_server.lock_mode;

  assert(mode == (struct hikari_lock_mode *)hikari_server.mode);

  render_background(renderer, 0.1);
  render_public_views(renderer);
  render_lock_indicator(renderer, mode->lock_indicator);
}

void
hikari_renderer_mark_assign_mode(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

  render_background(renderer, 1);
  render_workspace(renderer);

  struct hikari_mark_assign_mode *mode = &hikari_server.mark_assign_mode;

  assert(mode == (struct hikari_mark_assign_mode *)hikari_server.mode);
  assert(hikari_server.workspace->focus_view != NULL);

  struct hikari_view *view = hikari_server.workspace->focus_view;

  if (mode->pending_mark != NULL && mode->pending_mark->view != NULL &&
      mode->pending_mark->view->output == output) {
    renderer->geometry = hikari_view_border_geometry(mode->pending_mark->view);

    render_indicator_frame(&mode->pending_mark->view->indicator_frame,
        hikari_configuration->indicator_conflict,
        renderer);
    render_indicator(&mode->indicator, renderer);
  }

  if (view->output == output) {
    renderer->geometry = hikari_view_border_geometry(view);

    render_indicator_frame(&view->indicator_frame,
        hikari_configuration->indicator_selected,
        renderer);

    render_indicator(&hikari_server.indicator, renderer);
  }

#ifdef HAVE_LAYERSHELL
  render_overlay(renderer);
#endif
}

void
hikari_renderer_move_mode(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

  render_background(renderer, 1);
  render_workspace(renderer);

  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view->output == output && !hikari_view_is_hidden(focus_view)) {
    renderer->geometry = hikari_view_border_geometry(focus_view);

    render_indicator_frame(&focus_view->indicator_frame,
        hikari_configuration->indicator_insert,
        renderer);

    render_indicator(&hikari_server.indicator, renderer);
  }

#ifdef HAVE_LAYERSHELL
  render_overlay(renderer);
#endif
}

void
hikari_renderer_resize_mode(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

  render_background(renderer, 1);
  render_workspace(renderer);

  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view->output == output) {
    renderer->geometry = hikari_view_border_geometry(focus_view);

    render_indicator_frame(&focus_view->indicator_frame,
        hikari_configuration->indicator_insert,
        renderer);

    render_indicator(&hikari_server.indicator, renderer);
  }

#ifdef HAVE_LAYERSHELL
  render_overlay(renderer);
#endif
}

void
hikari_renderer_sheet_assign_mode(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

  render_background(renderer, 1);
  render_workspace(renderer);

  assert(hikari_server.workspace->focus_view != NULL);
  struct hikari_view *view = hikari_server.workspace->focus_view;

  if (view->output == output) {
    renderer->geometry = hikari_view_border_geometry(view);

    render_indicator_frame(&view->indicator_frame,
        hikari_configuration->indicator_selected,
        renderer);

    render_indicator(&hikari_server.indicator, renderer);
  }

#ifdef HAVE_LAYERSHELL
  render_overlay(renderer);
#endif
}

static inline void
render_default_workspace(struct hikari_renderer *renderer)
{
  render_background(renderer, 1);
  render_workspace(renderer);
#ifdef HAVE_LAYERSHELL
  render_overlay(renderer);
#endif
}

void
hikari_renderer_dnd_mode(struct hikari_renderer *renderer)
{
  render_default_workspace(renderer);
}

void
hikari_renderer_layout_select_mode(struct hikari_renderer *renderer)
{
  struct hikari_output *output = renderer->wlr_output->data;

  render_background(renderer, 1);
  render_workspace(renderer);

  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view != NULL && focus_view->output == output) {
    renderer->geometry = hikari_view_border_geometry(focus_view);

    render_indicator_frame(&focus_view->indicator_frame,
        hikari_configuration->indicator_selected,
        renderer);

    render_indicator(&hikari_server.indicator, renderer);
  }

#ifdef HAVE_LAYERSHELL
  render_overlay(renderer);
#endif
}

void
hikari_renderer_mark_select_mode(struct hikari_renderer *renderer)
{
  render_default_workspace(renderer);
}
