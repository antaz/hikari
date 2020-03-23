#include <hikari/font.h>

#include <cairo/cairo.h>
#include <string.h>

static void
font_metrics(
    struct hikari_font *font, const char *text, int *width, int *height)
{
  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);

  cairo_t *cairo = cairo_create(surface);
  PangoLayout *layout = pango_cairo_create_layout(cairo);

  pango_layout_set_font_description(layout, font->desc);
  pango_layout_set_text(layout, text, -1);
  pango_cairo_update_layout(cairo, layout);
  pango_cairo_show_layout(cairo, layout);
  pango_layout_get_pixel_size(layout, width, height);

  g_object_unref(layout);
  cairo_surface_destroy(surface);
  cairo_destroy(cairo);
}

void
hikari_font_init(struct hikari_font *font, const char *font_name)
{
  font->desc = pango_font_description_from_string(font_name);

  font_metrics(font, "W", &font->character_width, &font->height);

  // precalculate height for indicator bars since this is the only height we
  // need.
  font->height += 8;
}

void
hikari_font_fini(struct hikari_font *font)
{
  pango_font_description_free(font->desc);
}
