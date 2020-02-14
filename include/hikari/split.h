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

struct hikari_split {
  enum hikari_split_type type;
};

struct hikari_vertical_split {
  struct hikari_split split;

  float scale;

  struct hikari_split *left;
  struct hikari_split *right;
};

struct hikari_horizontal_split {
  struct hikari_split split;

  float scale;

  struct hikari_split *top;
  struct hikari_split *bottom;
};

struct hikari_container {
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
hikari_container_init(
    struct hikari_container *container, int nr_of_views, layout_func_t layout);

void
hikari_split_fini(struct hikari_split *split);

void
hikari_vertical_split_init(struct hikari_vertical_split *vertical_split,
    float scale,
    struct hikari_split *left,
    struct hikari_split *right);

void
hikari_horizontal_split_init(struct hikari_horizontal_split *horizontal_split,
    float scale,
    struct hikari_split *left,
    struct hikari_split *right);

void
hikari_split_render(
    struct hikari_split *split, struct hikari_render_data *render_data);

void
hikari_container_render(
    struct hikari_container *container, struct hikari_render_data *render_data);

#endif
