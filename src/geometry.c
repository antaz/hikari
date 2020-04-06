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
hikari_geometry_constrain_absolute(
    struct wlr_box *geometry, struct wlr_box *usable_area, int x, int y)
{
  int border = hikari_configuration->border;

  int usable_max_x = usable_area->x + usable_area->width;
  int usable_min_x = usable_area->x;
  int usable_max_y = usable_area->y + usable_area->height;
  int usable_min_y = usable_area->y;

  if (x + geometry->width + border > usable_max_x) {
    geometry->x = usable_max_x - geometry->width - border;
  } else if (x - border < usable_min_x) {
    geometry->x = usable_min_x + border;
  } else {
    geometry->x = x;
  }

  if (y + geometry->height + border > usable_max_y) {
    geometry->y = usable_max_y - geometry->height - border;
  } else if (y - border < usable_min_y) {
    geometry->y = usable_min_y + border;
  } else {
    geometry->y = y;
  }
}

void
hikari_geometry_constrain_relative(
    struct wlr_box *geometry, struct wlr_box *usable_area, int x, int y)
{
  int border = hikari_configuration->border;
  int gap = hikari_configuration->gap * 2 - border;

  int usable_max_x = usable_area->x + usable_area->width - gap;
  int usable_min_x = usable_area->x - geometry->width + gap;
  int usable_max_y = usable_area->y + usable_area->height - gap;
  int usable_min_y = usable_area->y - geometry->height + gap;

  if (x > usable_max_x) {
    geometry->x = usable_max_x;
  } else if (x < usable_min_x) {
    geometry->x = usable_min_x;
  } else {
    geometry->x = x;
  }

  if (y > usable_max_y) {
    geometry->y = usable_max_y;
  } else if (y < usable_min_y) {
    geometry->y = usable_min_y;
  } else {
    geometry->y = y;
  }
}
