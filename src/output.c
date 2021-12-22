#include <hikari/output.h>

#include <drm_fourcc.h>

#include <wlr/backend.h>

#include <hikari/memory.h>
#include <hikari/renderer.h>
#include <hikari/server.h>
#ifdef HAVE_XWAYLAND
#include <hikari/view.h>
#endif

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

  struct wlr_renderer *renderer = output->wlr_output->renderer;

  output->background = wlr_texture_from_pixels(
      renderer, DRM_FORMAT_ARGB8888, stride, output_width, output_height, data);

  cairo_surface_destroy(image);
  cairo_surface_destroy(output_surface);

done:
  if (output->enabled) {
    hikari_output_damage_whole(output);
  }
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

  if (!output->enabled) {
    return;
  }

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

  if (output->enabled) {
    return;
  }

  struct wlr_output *wlr_output = output->wlr_output;

  wl_list_remove(&output->damage_frame.link);
  output->damage_frame.notify = hikari_renderer_damage_frame_handler;
  wl_signal_add(&output->damage->events.frame, &output->damage_frame);

  wlr_output_enable(wlr_output, true);
  wlr_output_commit(wlr_output);
  hikari_output_damage_whole(output);

  output->enabled = true;
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

  hikari_output_disable(output);
}

#ifdef HAVE_LAYERSHELL
static void
close_layers(struct wl_list *layers)
{
  struct hikari_layer *layer, *layer_temp;
  wl_list_for_each_safe (layer, layer_temp, layers, layer_surfaces) {
    wlr_layer_surface_v1_destroy(layer->surface);
    layer->output = NULL;
  }
}
#endif

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_output *output = wl_container_of(listener, output, destroy);

#ifndef NDEBUG
  printf("DESTORY OUTPUT %p\n", output);
#endif

  hikari_output_fini(output);
  hikari_free(output);
}

void
hikari_output_init(struct hikari_output *output, struct wlr_output *wlr_output)
{
  assert(!output->enabled);

  bool noop = wlr_output->backend == hikari_server.noop_backend;

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

  output->destroy.notify = destroy_handler;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy);

  if (!noop) {
    bool first = wl_list_empty(&hikari_server.outputs);

    wl_list_insert(&hikari_server.outputs, &output->server_outputs);

    output->damage_destroy.notify = damage_destroy_handler;
    wl_signal_add(&output->damage->events.destroy, &output->damage_destroy);

    if (!wl_list_empty(&wlr_output->modes)) {
      struct wlr_output_mode *mode =
          wl_container_of(wlr_output->modes.next, mode, link);
      wlr_output_set_mode(wlr_output, mode);
    }

    wl_list_init(&output->damage_frame.link);

    if (!hikari_server_in_lock_mode()) {
      hikari_output_enable(output);
    } else if (hikari_lock_mode_are_outputs_disabled(
                   &hikari_server.lock_mode)) {
      hikari_output_disable(output);
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

    if (first) {
      hikari_workspace_merge(
          hikari_server.noop_output->workspace, output->workspace);
      hikari_workspace_focus_view(output->workspace, NULL);
    }
  }
}

void
hikari_output_fini(struct hikari_output *output)
{
  bool noop = output->wlr_output->backend == hikari_server.noop_backend;

#ifdef HAVE_LAYERSHELL
  close_layers(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY]);
  close_layers(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP]);
  close_layers(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM]);
  close_layers(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND]);
#endif

  hikari_output_disable(output);

  wl_list_remove(&output->destroy.link);

  struct hikari_workspace *workspace = output->workspace;

  if (!noop) {
    struct hikari_workspace *merge_workspace;
    struct hikari_workspace *next_workspace = hikari_workspace_next(workspace);

    wlr_texture_destroy(output->background);

    if (workspace != next_workspace) {
      merge_workspace = next_workspace;
    } else {
      merge_workspace = hikari_server.noop_output->workspace;
    }

    hikari_workspace_merge(workspace, merge_workspace);

    if (!hikari_server_in_lock_mode()) {
      if (!hikari_server_in_normal_mode()) {
        hikari_server_enter_normal_mode(NULL);
      }

      hikari_workspace_focus_view(merge_workspace, NULL);
    } else {
      merge_workspace->focus_view = NULL;
      hikari_server.workspace = merge_workspace;
    }

    wl_list_remove(&output->server_outputs);
    wl_list_remove(&output->damage_destroy.link);
  } else {
    hikari_server.workspace = NULL;
  }

  hikari_workspace_fini(workspace);
  hikari_free(workspace);
}

void
hikari_output_move(struct hikari_output *output, double lx, double ly)
{
  wlr_output_layout_move(
      hikari_server.output_layout, output->wlr_output, lx, ly);
}

#define CYCLE_OUTPUT(name)                                                     \
  struct hikari_output *hikari_output_##name(struct hikari_output *output)     \
  {                                                                            \
    if (wl_list_empty(&hikari_server.outputs)) {                               \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    struct wl_list *name = output->server_outputs.name;                        \
                                                                               \
    if (name == &hikari_server.outputs) {                                      \
      name = hikari_server.outputs.name;                                       \
    }                                                                          \
                                                                               \
    struct hikari_output *name##_output =                                      \
        wl_container_of(name, name##_output, server_outputs);                  \
                                                                               \
    return name##_output;                                                      \
  }

CYCLE_OUTPUT(next)
CYCLE_OUTPUT(prev)
#undef CYCLE_OUTPUT

#ifdef HAVE_XWAYLAND
void
hikari_output_rearrange_xwayland_views(struct hikari_output *output)
{
  struct hikari_view *view;
  wl_list_for_each (view, &output->views, output_views) {
    if (view->move != NULL) {
      struct wlr_box *geometry = hikari_view_geometry(view);
      view->move(view, geometry->x, geometry->y);
    }
  }
}
#endif
