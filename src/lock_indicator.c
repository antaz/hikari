#include <hikari/lock_indicator.h>

#include <drm_fourcc.h>

#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>

#include <hikari/configuration.h>
#include <hikari/geometry.h>
#include <hikari/output.h>
#include <hikari/server.h>

#define HIKARI_PI 3.14159265358979323846

static struct wlr_texture *
init_indicator_circle(float color[static 4])
{
  const int size = 100;

  struct wlr_texture *texture;
  struct wlr_renderer *wlr_renderer = hikari_server.renderer;

  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);

  cairo_t *cairo = cairo_create(surface);
  PangoLayout *layout = pango_cairo_create_layout(cairo);

  float *border_inactive = hikari_configuration->border_active;
  cairo_set_source_rgba(cairo,
      border_inactive[0],
      border_inactive[1],
      border_inactive[2],
      border_inactive[3]);
  cairo_set_line_width(cairo, 5);
  cairo_translate(cairo, size / 2, size / 2);
  cairo_arc(cairo, 0, 0, (size - 5) / 2, 0, 2 * HIKARI_PI);
  cairo_stroke_preserve(cairo);
  cairo_set_source_rgba(cairo, color[0], color[1], color[2], color[3]);
  cairo_fill(cairo);

  cairo_surface_flush(surface);

  unsigned char *data = cairo_image_surface_get_data(surface);
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, size);

  texture = wlr_texture_from_pixels(
      wlr_renderer, DRM_FORMAT_ARGB8888, stride, size, size, data);

  cairo_surface_destroy(surface);
  g_object_unref(layout);
  cairo_destroy(cairo);

  return texture;
}

static int
reset_state_handler(void *data)
{
  struct hikari_lock_indicator *lock_indicator = data;

  if (lock_indicator->current == lock_indicator->deny) {
    hikari_lock_indicator_clear(lock_indicator);
  } else {
    hikari_lock_indicator_set_wait(lock_indicator);
  }

  return 0;
}

void
hikari_lock_indicator_init(struct hikari_lock_indicator *lock_indicator)
{
  assert(lock_indicator != NULL);

  lock_indicator->wait = init_indicator_circle(hikari_configuration->clear);
  lock_indicator->type =
      init_indicator_circle(hikari_configuration->indicator_insert);
  lock_indicator->verify =
      init_indicator_circle(hikari_configuration->indicator_selected);
  lock_indicator->deny =
      init_indicator_circle(hikari_configuration->indicator_conflict);

  lock_indicator->current = NULL;

  lock_indicator->reset_state = wl_event_loop_add_timer(
      hikari_server.event_loop, reset_state_handler, lock_indicator);
}

void
hikari_lock_indicator_fini(struct hikari_lock_indicator *lock_indicator)
{
  assert(lock_indicator != NULL);

  wlr_texture_destroy(lock_indicator->wait);
  wlr_texture_destroy(lock_indicator->type);
  wlr_texture_destroy(lock_indicator->verify);
  wlr_texture_destroy(lock_indicator->deny);

  wl_event_source_remove(lock_indicator->reset_state);
}

void
hikari_lock_indicator_set_type(struct hikari_lock_indicator *lock_indicator)
{
  assert(lock_indicator != NULL);

  lock_indicator->current = lock_indicator->type;
  hikari_lock_indicator_damage(lock_indicator);
  wl_event_source_timer_update(lock_indicator->reset_state, 100);
}

void
hikari_lock_indicator_set_verify(struct hikari_lock_indicator *lock_indicator)
{
  assert(lock_indicator != NULL);

  lock_indicator->current = lock_indicator->verify;
  hikari_lock_indicator_damage(lock_indicator);
  wl_event_source_timer_update(lock_indicator->reset_state, 0);
}

void
hikari_lock_indicator_set_deny(struct hikari_lock_indicator *lock_indicator)
{
  assert(lock_indicator != NULL);

  lock_indicator->current = lock_indicator->deny;
  hikari_lock_indicator_damage(lock_indicator);
  wl_event_source_timer_update(lock_indicator->reset_state, 1000);
}

void
hikari_lock_indicator_set_wait(struct hikari_lock_indicator *lock_indicator)
{
  assert(lock_indicator != NULL);

  lock_indicator->current = lock_indicator->wait;
  hikari_lock_indicator_damage(lock_indicator);
}

void
hikari_lock_indicator_clear(struct hikari_lock_indicator *lock_indicator)
{
  assert(lock_indicator != NULL);

  lock_indicator->current = NULL;
  hikari_lock_indicator_damage(lock_indicator);
  wl_event_source_timer_update(lock_indicator->reset_state, 0);
}

static inline void
get_geometry(struct hikari_output *output, struct wlr_box *geometry)
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

void
hikari_lock_indicator_damage(struct hikari_lock_indicator *lock_indicator)
{
  assert(lock_indicator != NULL);

  struct wlr_box geometry;

  struct hikari_output *output;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    get_geometry(output, &geometry);
    hikari_output_add_damage(output, &geometry);
  }
}
