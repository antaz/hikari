#if !defined(HIKARI_FONT_H)
#define HIKARI_FONT_H

#include <pango/pangocairo.h>

struct hikari_font {
  PangoFontDescription *desc;

  int height;
  int character_width;
};

void
hikari_font_init(struct hikari_font *font, const char *font_name);

void
hikari_font_fini(struct hikari_font *font);

#endif
