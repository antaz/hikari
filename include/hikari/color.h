#if !defined(HIKARI_COLOR_H)
#define HIKARI_COLOR_H

#include <stdint.h>

static inline void
hikari_color_convert(float dst[static 4], uint32_t color)
{
  dst[0] = ((color >> 16) & 0xff) / 255.0;
  dst[1] = ((color >> 8) & 0xff) / 255.0;
  dst[2] = (color & 0xff) / 255.0;
  dst[3] = 1.0;
}

#endif
