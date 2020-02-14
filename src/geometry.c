#include <hikari/geometry.h>

#include <hikari/configuration.h>

#define SPLIT(n, x, y, width, height)                                          \
  void hikari_geometry_split_##n(struct wlr_box *src,                          \
      float scale,                                                             \
      int gap,                                                                 \
      struct wlr_box *dst1,                                                    \
      struct wlr_box *dst2)                                                    \
  {                                                                            \
    int width = (src->width - gap) * (1.0 - scale);                            \
    int rest = src->width - gap - width * 2;                                   \
                                                                               \
    dst1->x = src->x;                                                          \
    dst1->y = src->y;                                                          \
    dst1->width = width + rest;                                                \
    dst1->height = src->height;                                                \
                                                                               \
    dst2->x = src->x + dst1->width + gap;                                      \
    dst2->y = src->y;                                                          \
    dst2->width = width;                                                       \
    dst2->height = src->height;                                                \
  }

SPLIT(vertical, x, y, width, height)
SPLIT(horizontal, y, x, height, width)
#undef SPLIT

void
hikari_geometry_shrink(struct wlr_box *geometry, int gap)
{
  geometry->x += gap;
  geometry->y += gap;
  geometry->width -= gap * 2;
  geometry->height -= gap * 2;
}

void
hikari_geometry_constrain_position(
    struct wlr_box *geometry, int screen_width, int screen_height, int x, int y)
{
  if (x + geometry->width + hikari_configuration.border > screen_width) {
    geometry->x = screen_width - geometry->width - hikari_configuration.border;
  } else {
    geometry->x = x;
  }

  if (y + geometry->height + hikari_configuration.border > screen_height) {
    geometry->y =
        screen_height - geometry->height - hikari_configuration.border;
  } else {
    geometry->y = y;
  }
}
