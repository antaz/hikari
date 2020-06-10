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

#define CENTER(coord, dim)                                                     \
  static int center_##coord(                                                   \
      struct wlr_box *geometry, struct wlr_box *usable_area)                   \
  {                                                                            \
    int center_usable_area_##coord = usable_area->dim / 2;                     \
    int center_geometry_##coord = geometry->dim / 2;                           \
                                                                               \
    return usable_area->coord + center_usable_area_##coord -                   \
           center_geometry_##coord;                                            \
  }

CENTER(x, width)
CENTER(y, height)
#undef CENTER

static int
right_x(struct wlr_box *geometry, struct wlr_box *usable_area)
{
  return usable_area->x + usable_area->width - geometry->width +
         hikari_configuration->border;
}

static int
bottom_y(struct wlr_box *geometry, struct wlr_box *usable_area)
{
  return usable_area->y + usable_area->height - geometry->height +
         hikari_configuration->border;
}

void
hikari_geometry_position_bottom_left(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y)
{
  *x = usable_area->x;
  *y = bottom_y(geometry, usable_area);
}

void
hikari_geometry_position_bottom_middle(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y)
{
  *x = center_x(geometry, usable_area);
  *y = bottom_y(geometry, usable_area);
}

void
hikari_geometry_position_bottom_right(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y)
{
  *x = right_x(geometry, usable_area);
  *y = bottom_y(geometry, usable_area);
}

void
hikari_geometry_position_center(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y)
{
  *x = center_x(geometry, usable_area);
  *y = center_y(geometry, usable_area);
}

void
hikari_geometry_position_center_left(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y)
{
  *x = usable_area->x;
  *y = center_y(geometry, usable_area);
}

void
hikari_geometry_position_center_right(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y)
{
  *x = right_x(geometry, usable_area);
  *y = center_y(geometry, usable_area);
}

void
hikari_geometry_position_top_left(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y)
{
  *x = usable_area->x;
  *y = usable_area->y;
}

void
hikari_geometry_position_top_middle(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y)
{
  *x = center_x(geometry, usable_area);
  *y = usable_area->y;
}

void
hikari_geometry_position_top_right(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y)
{
  *x = right_x(geometry, usable_area);
  *y = usable_area->y;
}

void
hikari_geometry_constrain_size(struct wlr_box *geometry,
    struct wlr_box *usable_area,
    struct wlr_box *constrained)
{
  if (geometry->x < 0) {
    constrained->width = geometry->width + geometry->x;
    constrained->x = 0;
  } else {
    int excess_width = geometry->width - (usable_area->width - geometry->x);

    if (excess_width > 0) {
      constrained->width = geometry->width - excess_width;
    } else {
      constrained->width = geometry->width;
    }

    constrained->x = geometry->x;
  }

  if (geometry->y < 0) {
    constrained->height = geometry->height + geometry->y;
    constrained->y = 0;
  } else {
    int excess_height = geometry->height - (usable_area->height - geometry->y);

    if (excess_height > 0) {
      constrained->height = geometry->height - excess_height;
    } else {
      constrained->height = geometry->height;
    }

    constrained->y = geometry->y;
  }
}
