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
render_image_to_surface(cairo_surface_t *output,
    cairo_surface_t *image,
    enum hikari_background_fit fit)
{
  cairo_t *cairo = cairo_create(output);
  if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
    return;
  }

  double output_width = cairo_image_surface_get_width(output);
  double output_height = cairo_image_surface_get_height(output);
  double width = cairo_image_surface_get_width(image);
  double height = cairo_image_surface_get_height(image);

  cairo_rectangle(cairo, 0, 0, output_width, output_height);
  cairo_fill(cairo);

  if (fit == HIKARI_BACKGROUND_STRETCH) {
    cairo_scale(cairo, output_width / width, output_height / height);
    cairo_set_source_surface(cairo, image, 0, 0);
  } else if (fit == HIKARI_BACKGROUND_CENTER) {
    cairo_set_source_surface(cairo,
        image,
        output_width / 2 - width / 2,
        output_height / 2 - height / 2);
  } else if (fit == HIKARI_BACKGROUND_TILE) {
    cairo_pattern_t *pattern = cairo_pattern_create_for_surface(image);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
    cairo_set_source(cairo, pattern);
    cairo_pattern_destroy(pattern);
  }

  cairo_paint(cairo);
  cairo_destroy(cairo);
}

void
hikari_output_load_background(struct hikari_output *output,
    const char *path,
    enum hikari_background_fit background_fit)
{
  if (output->background != NULL) {
    wlr_texture_destroy(output->background);
    output->background = NULL;
  }

  assert(output->background == NULL);

  if (path == NULL) {
    goto done;
  }

  cairo_surface_t *image = cairo_image_surface_create_from_png(path);
  if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
    goto done;
  }

  int output_width = output->geometry.width;
  int output_height = output->geometry.height;

  cairo_surface_t *output_surface = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32, output_width, output_height);
  if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(image);
    goto done;
  }

  render_image_to_surface(output_surface, image, background_fit);

  unsigned char *data = cairo_image_surface_get_data(output_surface);
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, output_width);

  struct wlr_renderer *renderer =
      wlr_backend_get_renderer(output->wlr_output->backend);

  output->background = wlr_texture_from_pixels(renderer,
      WL_SHM_FORMAT_ARGB8888,
      stride,
      output_width,
      output_height,
      data);

  cairo_surface_destroy(image);
  cairo_surface_destroy(output_surface);

done:
  if (output->enabled) {
    hikari_output_damage_whole(output);
  }
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
render_background(
    struct hikari_output *output, struct hikari_render_data *render_data)
{
  if (output->background == NULL) {
    return;
  }

  float matrix[9];
  struct wlr_output *wlr_output = output->wlr_output;

  struct wlr_box geometry = { .x = 0, .y = 0 };
  wlr_output_transformed_resolution(
      wlr_output, &geometry.width, &geometry.height);

  wlr_matrix_project_box(matrix, &geometry, 0, 0, wlr_output->transform_matrix);

  render_texture(wlr_output,
      render_data->damage,
      output->background,
      render_data->renderer,
      matrix,
      &geometry);
}

#ifdef HAVE_LAYERSHELL
static void
layer_for_each(struct wl_list *layers,
    void (*func)(struct wlr_surface *, int, int, void *),
    void *data)
{
  struct hikari_layer *layer;
  wl_list_for_each (layer, layers, layer_surfaces) {
    wlr_layer_surface_v1_for_each_surface(layer->surface, func, data);
  }
}

static void
render_layer(struct wl_list *layers, struct hikari_render_data *render_data)
{
  struct hikari_layer *layer;
  wl_list_for_each (layer, layers, layer_surfaces) {
    render_data->geometry = &layer->geometry;
    wlr_layer_surface_v1_for_each_surface(
        layer->surface, render_surface, render_data);
  }
}
#endif

static void
render_output(struct hikari_output *output,
    pixman_region32_t *damage,
    struct timespec *now)
{
  struct wlr_output *wlr_output = output->wlr_output;

  struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);

  wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);

  if (!pixman_region32_not_empty(damage)) {
    goto render_end;
  }

  float *clear_color = hikari_configuration->clear;

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

  render_background(output, &render_data);

#ifdef HAVE_LAYERSHELL
  render_layer(
      &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND], &render_data);
  render_layer(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM], &render_data);
#endif

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

#ifdef HAVE_LAYERSHELL
  render_layer(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP], &render_data);
#endif

#ifdef HAVE_XWAYLAND
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view = NULL;
  wl_list_for_each_reverse (xwayland_unmanaged_view,
      &output->unmanaged_xwayland_views,
      unmanaged_server_views) {

    render_data.geometry = &xwayland_unmanaged_view->geometry;

    wlr_surface_for_each_surface(xwayland_unmanaged_view->surface->surface,
        render_surface,
        &render_data);
  }
#endif

  hikari_server.mode->render(output, &render_data);

#ifdef HAVE_LAYERSHELL
  render_layer(
      &output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY], &render_data);
#endif

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
  wl_list_for_each_reverse (view, &output->views, output_views) {
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

#ifdef HAVE_LAYERSHELL
  for (int i = 0; i < 4; i++) {
    layer_for_each(&output->layers[i], send_frame_done, &now);
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
  assert(output->enabled);

  struct wlr_output *wlr_output = output->wlr_output;

  wl_list_remove(&output->damage_frame.link);

  wlr_output_rollback(wlr_output);
  wlr_output_enable(wlr_output, false);
  wlr_output_commit(wlr_output);

  output->enabled = false;
}

void
hikari_output_enable(struct hikari_output *output)
{
  assert(output != NULL);
  assert(!output->enabled);

  struct wlr_output *wlr_output = output->wlr_output;

  output->damage_frame.notify = damage_frame_handler;
  wl_signal_add(&output->damage->events.frame, &output->damage_frame);

  wlr_output_enable(wlr_output, true);
  wlr_output_commit(wlr_output);
  hikari_output_damage_whole(output);

  output->enabled = true;
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
  struct wlr_box *output_box = wlr_output_layout_get_box(
      hikari_server.output_layout, output->wlr_output);

  output->geometry.x = output_box->x;
  output->geometry.y = output_box->y;
  output->geometry.width = output_box->width;
  output->geometry.height = output_box->height;

  output->usable_area = (struct wlr_box){
    .x = 0, .y = 0, .width = output_box->width, .height = output_box->height
  };
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
damage_destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_output *output =
      wl_container_of(listener, output, damage_destroy);

  if (output->enabled) {
    hikari_output_disable(output);
  }
}

#ifdef HAVE_LAYERSHELL
static void
close_layers(struct wl_list *layers)
{
  struct hikari_layer *layer, *layer_temp;
  wl_list_for_each_safe (layer, layer_temp, layers, layer_surfaces) {
    wlr_layer_surface_v1_close(layer->surface);
    layer->output = NULL;
  }
}
#endif

void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_output *output = wl_container_of(listener, output, destroy);

  struct hikari_workspace *workspace = output->workspace;
  struct hikari_workspace *next = hikari_workspace_next(workspace);

#ifndef NDEBUG
  printf("DESTORY OUTPUT %p\n", output);
#endif

#ifdef HAVE_LAYERSHELL
  close_layers(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY]);
  close_layers(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP]);
  close_layers(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM]);
  close_layers(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND]);
#endif

  if (workspace != next) {
    hikari_workspace_merge(workspace, next);
  }

  hikari_output_fini(output);
  hikari_free(output);
}

void
hikari_output_init(struct hikari_output *output, struct wlr_output *wlr_output)
{
  output->wlr_output = wlr_output;
  output->damage = wlr_output_damage_create(wlr_output);
  output->background = NULL;
  output->enabled = false;
  output->workspace = hikari_malloc(sizeof(struct hikari_workspace));

#ifdef HAVE_XWAYLAND
  wl_list_init(&output->unmanaged_xwayland_views);
#endif
  wl_list_init(&output->views);

#ifdef HAVE_LAYERSHELL
  wl_list_init(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY]);
  wl_list_init(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP]);
  wl_list_init(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM]);
  wl_list_init(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND]);
#endif

  hikari_workspace_init(output->workspace, output);

  wlr_output->data = output;

  wl_list_insert(&hikari_server.outputs, &output->server_outputs);

  /* output->mode.notify = mode_handler; */
  /* wl_signal_add(&wlr_output->events.mode, &output->mode); */

  output->destroy.notify = destroy_handler;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy);

  output->damage_destroy.notify = damage_destroy_handler;
  wl_signal_add(&output->damage->events.destroy, &output->damage_destroy);

  if (!wl_list_empty(&wlr_output->modes)) {
    struct wlr_output_mode *mode =
        wl_container_of(wlr_output->modes.prev, mode, link);
    wlr_output_set_mode(wlr_output, mode);
  }

  if (!hikari_server.locked) {
    hikari_output_enable(output);
  }

  struct hikari_output_config *output_config =
      hikari_configuration_resolve_output(
          hikari_configuration, wlr_output->name);

  if (output_config != NULL && output_config->explicit_position) {
    wlr_output_layout_add(hikari_server.output_layout,
        wlr_output,
        output_config->lx,
        output_config->ly);
  } else {
    wlr_output_layout_add_auto(hikari_server.output_layout, wlr_output);
  }

  output_geometry(output);
}

void
hikari_output_fini(struct hikari_output *output)
{
  if (output->enabled) {
    hikari_output_disable(output);
  }

  wlr_texture_destroy(output->background);

  wl_list_remove(&output->destroy.link);
  /* wl_list_remove(&output->mode.link); */

  wl_list_remove(&output->server_outputs);

  struct hikari_workspace *new_workspace = hikari_server.workspace;
  if (output->workspace == hikari_server.workspace) {
    struct hikari_workspace *workspace;
    wl_list_for_each (workspace, &hikari_server.workspaces, server_workspaces) {
      if (workspace != output->workspace) {
        new_workspace = workspace;
        break;
      }
    }
  }

  if (new_workspace != NULL) {
    hikari_workspace_focus_view(new_workspace, NULL);
  } else {
    assert(false);
    hikari_server.workspace = NULL;
  }

  hikari_workspace_fini(output->workspace);
  hikari_free(output->workspace);
}

void
hikari_output_move(struct hikari_output *output, double lx, double ly)
{
  wlr_output_layout_move(
      hikari_server.output_layout, output->wlr_output, lx, ly);
}
