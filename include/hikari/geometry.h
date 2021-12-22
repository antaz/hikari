#if !defined(HIKARI_GEOMETRY_H)
#define HIKARI_GEOMETRY_H

#include <wlr/util/box.h>

void
hikari_geometry_split_vertical(struct wlr_box *src,
    int width,
    int gap,
    struct wlr_box *left,
    struct wlr_box *right);

void
hikari_geometry_split_horizontal(struct wlr_box *src,
    int height,
    int gap,
    struct wlr_box *top,
    struct wlr_box *bottom);

void
hikari_geometry_shrink(struct wlr_box *geometry, int gap);

void
hikari_geometry_constrain_absolute(
    struct wlr_box *geometry, struct wlr_box *usable_area, int x, int y);

void
hikari_geometry_constrain_relative(
    struct wlr_box *geometry, struct wlr_box *usable_area, int x, int y);

void
hikari_geometry_position_bottom_left(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y);

void
hikari_geometry_position_bottom_middle(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y);

void
hikari_geometry_position_bottom_right(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y);

void
hikari_geometry_position_center(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y);

void
hikari_geometry_position_center_left(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y);

void
hikari_geometry_position_center_right(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y);

void
hikari_geometry_position_top_left(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y);

void
hikari_geometry_position_top_middle(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y);

void
hikari_geometry_position_top_right(
    struct wlr_box *geometry, struct wlr_box *usable_area, int *x, int *y);

void
hikari_geometry_constrain_size(struct wlr_box *geometry,
    struct wlr_box *usable_area,
    struct wlr_box *constrained);

#define FIXED_SCALE(name)                                                      \
  static inline int hikari_geometry_scale_fixed_##name(                        \
      struct wlr_box *src, double scale, int gap)                              \
  {                                                                            \
    return (src->name - gap) * scale;                                          \
  }

FIXED_SCALE(width)
FIXED_SCALE(height)
#undef FIXED_SCALE

int
hikari_geometry_scale_dynamic_width(struct wlr_box *src,
    struct wlr_box *geometry,
    double min_scale,
    double max_scale,
    int gap);

int
hikari_geometry_scale_dynamic_height(struct wlr_box *src,
    struct wlr_box *geometry,
    double min_scale,
    double max_scale,
    int gap);

#endif
