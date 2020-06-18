#include <hikari/output.h>

#include <wlr/backend.h>

#include <hikari/memory.h>
#include <hikari/render.h>
#include <hikari/server.h>

static inline void
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

void
hikari_output_disable_content(struct hikari_output *output)
{
  wl_list_remove(&output->damage_frame.link);
  output->damage_frame.notify = damage_empty_frame_handler;
  wl_signal_add(&output->damage->events.frame, &output->damage_frame);

  hikari_output_damage_whole(output);
}

void
hikari_output_enable_content(struct hikari_output *output)
{
  wl_list_remove(&output->damage_frame.link);
  output->damage_frame.notify = damage_frame_handler;
  wl_signal_add(&output->damage->events.frame, &output->damage_frame);

  hikari_output_damage_whole(output);
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
  wl_list_init(&output->damage_frame.link);

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

  wl_list_init(&output->damage_frame.link);

  if (!hikari_server_in_lock_mode()) {
    hikari_output_enable_content(output);
    hikari_output_enable(output);
  } else {
    hikari_output_disable_content(output);
    if (hikari_lock_mode_are_outputs_disabled(&hikari_server.lock_mode)) {
      hikari_output_disable(output);
    }
  }

  struct hikari_output_config *output_config =
      hikari_configuration_resolve_output_config(
          hikari_configuration, wlr_output->name);

  if (output_config != NULL && output_config->position.value.type ==
                                   HIKARI_POSITION_CONFIG_TYPE_ABSOLUTE) {
    int x = output_config->position.value.config.absolute.x;
    int y = output_config->position.value.config.absolute.y;

    wlr_output_layout_add(hikari_server.output_layout, wlr_output, x, y);
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
