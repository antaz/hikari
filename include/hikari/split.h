#if !defined(HIKARI_SPLIT_H)
#define HIKARI_SPLIT_H

#include <wlr/types/wlr_box.h>

struct hikari_render_data;
struct hikari_sheet;
struct hikari_view;

typedef struct hikari_view *(*layout_func_t)(
    struct hikari_sheet *, struct hikari_view *, struct wlr_box *, int);

enum hikari_split_type {
  HIKARI_SPLIT_TYPE_VERTICAL,
  HIKARI_SPLIT_TYPE_HORIZONTAL,
  HIKARI_SPLIT_TYPE_CONTAINER
};

enum hikari_split_vertical_orientation {
  HIKARI_VERTICAL_SPLIT_ORIENTATION_LEFT,
  HIKARI_VERTICAL_SPLIT_ORIENTATION_RIGHT
};

enum hikari_split_horizontal_orientation {
  HIKARI_HORIZONTAL_SPLIT_ORIENTATION_TOP,
  HIKARI_HORIZONTAL_SPLIT_ORIENTATION_BOTTOM
};

enum hikari_split_scale_type {
  HIKARI_SPLIT_SCALE_TYPE_FIXED,
  HIKARI_SPLIT_SCALE_TYPE_DYNAMIC,

};

struct hikari_split_scale_dynamic {
  double min;
  double max;
};

struct hikari_split_scale {
  enum hikari_split_scale_type type;

  union {
    double fixed;
    struct hikari_split_scale_dynamic dynamic;
  } scale;
};

extern const double hikari_split_scale_min;
extern const double hikari_split_scale_max;
extern const double hikari_split_scale_default;

struct hikari_split {
  enum hikari_split_type type;
};

struct hikari_split_vertical {
  struct hikari_split split;
  enum hikari_split_vertical_orientation orientation;

  struct hikari_split_scale scale;

  struct hikari_split *left;
  struct hikari_split *right;
};

struct hikari_split_horizontal {
  struct hikari_split split;
  enum hikari_split_horizontal_orientation orientation;

  struct hikari_split_scale scale;

  struct hikari_split *top;
  struct hikari_split *bottom;
};

struct hikari_split_container {
  struct hikari_split split;
  struct wlr_box geometry;

  int max;

  layout_func_t layout;
};

void
hikari_split_apply(struct hikari_split *split,
    struct wlr_box *geometry,
    struct hikari_view *first);

void
hikari_split_container_init(struct hikari_split_container *container,
    int nr_of_views,
    layout_func_t layout);

void
hikari_split_fini(struct hikari_split *split);

void
hikari_split_vertical_init(struct hikari_split_vertical *split_vertical,
    struct hikari_split_scale *scale,
    enum hikari_split_vertical_orientation orientation,
    struct hikari_split *left,
    struct hikari_split *right);

void
hikari_split_horizontal_init(struct hikari_split_horizontal *split_horizontal,
    struct hikari_split_scale *scale,
    enum hikari_split_horizontal_orientation orientation,
    struct hikari_split *left,
    struct hikari_split *right);

#endif
