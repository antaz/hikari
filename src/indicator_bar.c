#include <hikari/indicator_bar.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <string.h>

#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_matrix.h>

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/font.h>
#include <hikari/indicator.h>
#include <hikari/output.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/view.h>

void
hikari_indicator_bar_init(struct hikari_indicator_bar *indicator_bar,
    struct hikari_indicator *indicator,
    int offset)
{
  indicator_bar->texture = NULL;
  indicator_bar->indicator = indicator;
  indicator_bar->offset = offset;
}

void
hikari_indicator_bar_fini(struct hikari_indicator_bar *indicator_bar)
{
  wlr_texture_destroy(indicator_bar->texture);
  indicator_bar->texture = NULL;
}

void
hikari_indicator_bar_damage(struct hikari_indicator_bar *indicator_bar,
    struct wlr_box *view_geometry,
    struct hikari_output *output)
{
  struct wlr_box geometry = { .x = view_geometry->x + 5,
    .y = view_geometry->y + indicator_bar->offset,
    .width = indicator_bar->width,
    .height = hikari_configuration->font.height };

  hikari_output_add_damage(output, &geometry);
}

void
hikari_indicator_bar_update(struct hikari_indicator_bar *indicator_bar,
    struct wlr_box *view_geometry,
    struct hikari_output *output,
    const char *text,
    float background[static 4])
{
  if (indicator_bar->texture != NULL) {
    hikari_indicator_bar_fini(indicator_bar);
  }

  if (text == NULL || !strcmp(text, "")) {
    return;
  }

  size_t len = strlen(text);
  struct hikari_font *font = &hikari_configuration->font;
  int width = hikari_configuration->font.character_width * len + 8;
  int height = hikari_configuration->font.height;

  struct wlr_box geometry = { .x = view_geometry->x + 5,
    .y = view_geometry->y + indicator_bar->offset,
    .width = indicator_bar->width,
    .height = height };

  hikari_output_add_damage(output, &geometry);

  geometry.width = width;
  indicator_bar->width = width;

  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

  cairo_t *cairo = cairo_create(surface);
  PangoLayout *layout = pango_cairo_create_layout(cairo);

  cairo_set_source_rgba(
      cairo, background[0], background[1], background[2], background[3]);
  cairo_paint(cairo);

  float *border_inactive = hikari_configuration->border_inactive;
  cairo_set_source_rgba(cairo,
      border_inactive[0],
      border_inactive[1],
      border_inactive[2],
      border_inactive[3]);
  cairo_rectangle(cairo, 0, 0, width, height);
  cairo_set_line_width(cairo, 1);
  cairo_stroke(cairo);

  cairo_set_source_rgba(cairo, 0, 0, 0, 1);
  pango_layout_set_font_description(layout, font->desc);
  cairo_move_to(cairo, 4, 4);
  pango_layout_set_text(layout, text, -1);

  pango_cairo_update_layout(cairo, layout);
  pango_cairo_show_layout(cairo, layout);

  cairo_surface_flush(surface);

  unsigned char *data = cairo_image_surface_get_data(surface);
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

  struct wlr_renderer *renderer = wlr_backend_get_renderer(
      hikari_server.workspace->output->output->backend);

  indicator_bar->texture = wlr_texture_from_pixels(
      renderer, WL_SHM_FORMAT_ARGB8888, stride, width, height, data);

  cairo_surface_destroy(surface);
  g_object_unref(layout);
  cairo_destroy(cairo);

  hikari_output_add_damage(output, &geometry);
}

void
hikari_indicator_bar_render(struct hikari_indicator_bar *indicator_bar,
    struct hikari_render_data *render_data)
{
  if (indicator_bar->texture == NULL) {
    return;
  }

  struct wlr_box *geometry = render_data->geometry;
  struct wlr_renderer *renderer = render_data->renderer;
  struct wlr_output *output = render_data->output;

  float matrix[9];

  geometry->width = indicator_bar->width;
  geometry->height = hikari_configuration->font.height;

  wlr_renderer_scissor(renderer, geometry);
  wlr_matrix_project_box(matrix, geometry, 0, 0, output->transform_matrix);

  wlr_render_texture_with_matrix(renderer, indicator_bar->texture, matrix, 1);
}
