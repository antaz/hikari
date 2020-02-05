#if !defined(HIKARI_FONT_H)
#define HIKARI_FONT_H

#include <pango/pangocairo.h>

struct hikari_font {
  PangoFontDescription *desc;
};

void
hikari_font_init(struct hikari_font *font, const char *font_name);

void
hikari_font_fini(struct hikari_font *font);

void
hikari_font_metrics(
    struct hikari_font *font, const char *text, int *width, int *height);

#endif
