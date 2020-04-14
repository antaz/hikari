#include <hikari/geometry.h>

#include <hikari/configuration.h>

#define SPLIT(n, x, y, width, height)                                          \
  void hikari_geometry_split_##n(struct wlr_box *src,                          \
      int width,                                                               \
      int gap,                                                                 \
      struct wlr_box *dst1,                                                    \
      struct wlr_box *dst2)                                                    \
  {                                                                            \
    int rest = src->width - gap - width;                                       \
                                                                               \
    dst1->x = src->x;                                                          \
    dst1->y = src->y;                                                          \
    dst1->width = width;                                                       \
    dst1->height = src->height;                                                \
                                                                               \
    dst2->x = src->x + width + gap;                                            \
    dst2->y = src->y;                                                          \
    dst2->width = rest;                                                        \
    dst2->height = src->height;                                                \
  }

SPLIT(vertical, x, y, width, height)
SPLIT(horizontal, y, x, height, width)
#undef SPLIT

#define DYNAMIC_SCALE(name)                                                    \
  int hikari_geometry_scale_dynamic_##name(struct wlr_box *src,                \
      struct wlr_box *geometry,                                                \
      double min_scale,                                                        \
      double max_scale,                                                        \
      int gap)                                                                 \
  {                                                                            \
    int min_##name = (src->name - gap) * min_scale;                            \
    int max_##name = (src->name - gap) * max_scale;                            \
    int name = geometry->name;                                                 \
                                                                               \
    if (name < min_##name) {                                                   \
      return min_##name;                                                       \
    } else if (name > max_##name) {                                            \
      return max_##name;                                                       \
    } else {                                                                   \
      return name;                                                             \
    }                                                                          \
  }

DYNAMIC_SCALE(width)
DYNAMIC_SCALE(height)
#undef DYNAMIC_SCALE

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
