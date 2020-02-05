#include <hikari/output.h>

#include <stdlib.h>

#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/region.h>
#ifdef HAVE_XWAYLAND
#include <wlr/xwayland.h>
#endif

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/indicator.h>
#include <hikari/indicator_bar.h>
#include <hikari/indicator_frame.h>
#include <hikari/memory.h>
#include <hikari/mode.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/workspace.h>
#include <hikari/xdg_view.h>
#ifdef HAVE_XWAYLAND
#include <hikari/xwayland_unmanaged_view.h>
#include <hikari/xwayland_view.h>
#endif

static void
load_background(struct hikari_workspace *workspace, const char *path)
{
  cairo_surface_t *image;

  image = cairo_image_surface_create_from_png(path);

  int width = workspace->output->geometry.width;
  int height = workspace->output->geometry.height;

  unsigned char *data = cairo_image_surface_get_data(image);
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

  struct wlr_renderer *renderer =
      wlr_backend_get_renderer(workspace->output->output->backend);

  workspace->background = wlr_texture_from_pixels(
      renderer, WL_SHM_FORMAT_ARGB8888, stride, width, height, data);

  cairo_surface_destroy(image);
}

static void
render_texture(struct wlr_output *output,
    pixman_region32_t *damage,
    struct wlr_texture *texture,
    struct wlr_renderer *renderer,
    const float matrix[static 9],
    struct wlr_box *box)
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
    hikari_output_scissor_render(output, renderer, &rects[i]);
    wlr_render_texture_with_matrix(renderer, texture, matrix, 1);
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

  struct hikari_render_data *render_data = data;
  struct wlr_box *geometry = render_data->geometry;
  struct wlr_output *wlr_output = render_data->output;

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

  render_texture(wlr_output,
      render_data->damage,
      texture,
      render_data->renderer,
      matrix,
      &box);
}

static void
render_output(struct hikari_output *output,
    pixman_region32_t *damage,
    struct timespec *now)
{
  struct wlr_renderer *renderer =
      wlr_backend_get_renderer(output->output->backend);

  struct wlr_output *wlr_output = output->output;

  wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);

  if (!pixman_region32_not_empty(damage)) {
    goto render_end;
  }

  float *clear_color = hikari_configuration.clear;

#ifndef NDEBUG
  if (hikari_server.track_damage) {
    float damage_color[4];
    hikari_color_convert(damage_color, 0x000000);
    wlr_renderer_clear(renderer, damage_color);
  }
#endif

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(damage, &nrects);
  for (int i = 0; i < nrects; ++i) {
    hikari_output_scissor_render(wlr_output, renderer, &rects[i]);
    wlr_renderer_clear(renderer, clear_color);
  }

  struct hikari_render_data render_data = {
    .output = wlr_output, .renderer = renderer, .when = now, .damage = damage
  };

  hikari_workspace_render_background(output->workspace, &render_data);

  struct hikari_view *view = NULL;
  wl_list_for_each_reverse (view, &output->workspace->views, workspace_views) {
    render_data.geometry = hikari_view_border_geometry(view);

    if (hikari_view_wants_border(view)) {
      hikari_border_render(&view->border, &render_data);
    }

    render_data.geometry = hikari_view_geometry(view);

    hikari_view_interface_for_each_surface(
        (struct hikari_view_interface *)view, render_surface, &render_data);
  }

#ifdef HAVE_XWAYLAND
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view = NULL;
  wl_list_for_each_reverse (xwayland_unmanaged_view,
      &output->unmanaged_xwayland_views,
      unmanaged_server_views) {

    render_data.geometry = xwayland_unmanaged_view->geometry;

    wlr_surface_for_each_surface(xwayland_unmanaged_view->surface->surface,
        render_surface,
        &render_data);
  }
#endif

  hikari_server.mode->render(output, &render_data);

render_end:
  wlr_renderer_scissor(renderer, NULL);
  wlr_output_render_software_cursors(wlr_output, NULL);
  wlr_renderer_end(renderer);

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

static void
send_frame_done(struct wlr_surface *surface, int sx, int sy, void *data)
{
  assert(surface != NULL);

  struct timespec *now = data;
  wlr_surface_send_frame_done(surface, now);
}

static void
damage_frame_handler(struct wl_listener *listener, void *data)
{
  assert(!hikari_server.locked);

  bool needs_frame;
  struct timespec now;
  struct hikari_output *output =
      wl_container_of(listener, output, damage_frame);

  pixman_region32_t buffer_damage;
  pixman_region32_init(&buffer_damage);

  if (!wlr_output_damage_attach_render(
          output->damage, &needs_frame, &buffer_damage)) {
    goto buffer_damage_end;
  }

  if (needs_frame) {
    clock_gettime(CLOCK_MONOTONIC, &now);
    render_output(output, &buffer_damage, &now);
  }

buffer_damage_end:
  pixman_region32_fini(&buffer_damage);

  struct hikari_view *view = NULL;
  wl_list_for_each_reverse (view, &output->workspace->views, workspace_views) {
    hikari_view_interface_for_each_surface(
        (struct hikari_view_interface *)view, send_frame_done, &now);
  }

#ifdef HAVE_XWAYLAND
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view = NULL;
  wl_list_for_each_reverse (xwayland_unmanaged_view,
      &output->unmanaged_xwayland_views,
      unmanaged_server_views) {
    wlr_surface_for_each_surface(
        xwayland_unmanaged_view->surface->surface, send_frame_done, &now);
  }
#endif
}

void
hikari_output_damage_whole(struct hikari_output *output)
{
  assert(output != NULL);

  wlr_output_damage_add_whole(output->damage);
}

void
hikari_output_disable(struct hikari_output *output)
{
  assert(output != NULL);

  struct wlr_output *wlr_output = output->output;

  wl_list_remove(&output->damage_frame.link);

  wlr_output_rollback(wlr_output);
  wlr_output_enable(wlr_output, false);
  wlr_output_commit(wlr_output);
}

void
hikari_output_enable(struct hikari_output *output)
{
  assert(output != NULL);

  output->damage_frame.notify = damage_frame_handler;
  wl_signal_add(&output->damage->events.frame, &output->damage_frame);

  wlr_output_enable(output->output, true);
  wlr_output_commit(output->output);
  hikari_output_damage_whole(output);
}

void
hikari_output_scissor_render(struct wlr_output *wlr_output,
    struct wlr_renderer *renderer,
    pixman_box32_t *rect)
{
  assert(wlr_output != NULL);

  struct wlr_box box = { .x = rect->x1,
    .y = rect->y1,
    .width = rect->x2 - rect->x1,
    .height = rect->y2 - rect->y1 };

  /* int ow, oh; */
  /* wlr_output_transformed_resolution(wlr_output, &ow, &oh); */

  /* enum wl_output_transform transform = */
  /*   wlr_output_transform_invert(wlr_output->transform); */
  /* wlr_box_transform(&box, &box, transform, ow, oh); */

  wlr_renderer_scissor(renderer, &box);
}

static void
output_geometry(struct hikari_output *output)
{
  struct wlr_box *output_box =
      wlr_output_layout_get_box(hikari_server.output_layout, output->output);

  output->geometry.x = output_box->x;
  output->geometry.y = output_box->y;
  output->geometry.width = output_box->width;
  output->geometry.height = output_box->height;
}

/* static void */
/* mode_handler(struct wl_listener *listener, void *data) */
/* { */
/* #if !defined(NDEBUG) */
/*   printf("MODE\n"); */
/* #endif */

/*   struct hikari_output *output = wl_container_of( */
/*       listener, */
/*       output, */
/*       mode); */

/*   output_geometry(output); */
/* } */

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_output *output = wl_container_of(listener, output, destroy);

  hikari_output_fini(output);
  hikari_free(output);
}

void
hikari_output_init(struct hikari_output *output, struct wlr_output *wlr_output)
{
  output->output = wlr_output;
  output->damage = wlr_output_damage_create(wlr_output);

#ifdef HAVE_XWAYLAND
  wl_list_init(&output->unmanaged_xwayland_views);
#endif
  wl_list_init(&output->views);

  /* output->mode.notify = mode_handler; */
  /* wl_signal_add(&wlr_output->events.mode, &output->mode); */
  output->destroy.notify = destroy_handler;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy);

  wlr_output_layout_add_auto(hikari_server.output_layout, wlr_output);
  wlr_output_create_global(wlr_output);

  output->workspace = hikari_malloc(sizeof(struct hikari_workspace));
  hikari_workspace_init(output->workspace, output);

  wl_list_insert(&hikari_server.outputs, &output->server_outputs);

  if (!wl_list_empty(&wlr_output->modes)) {
    struct wlr_output_mode *mode =
        wl_container_of(wlr_output->modes.prev, mode, link);
    wlr_output_set_mode(wlr_output, mode);
  }

  wlr_output->data = output;

  output_geometry(output);

  char *background = hikari_configuration_resolve_background(
      &hikari_configuration, wlr_output->name);

  hikari_output_enable(output);

  if (background != NULL) {
    load_background(output->workspace, background);
  }
}

void
hikari_output_fini(struct hikari_output *output)
{
  wl_list_remove(&output->damage_frame.link);
  /* wl_list_remove(&output->mode.link); */
  wl_list_remove(&output->destroy.link);

  hikari_workspace_fini(output->workspace);
  hikari_free(output->workspace);
}
