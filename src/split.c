#include <hikari/split.h>

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/geometry.h>
#include <hikari/view.h>

const double hikari_split_scale_min = 0.1;
const double hikari_split_scale_max = 0.9;
const double hikari_split_scale_default = 0.5;

static void
flip_scale(struct hikari_split_scale *scale)
{
  switch (scale->type) {
    case HIKARI_SPLIT_SCALE_TYPE_FIXED:
      scale->scale.fixed = 1.0 - scale->scale.fixed;
      break;

    case HIKARI_SPLIT_SCALE_TYPE_DYNAMIC:
      scale->scale.dynamic.min = 1.0 - scale->scale.dynamic.min;
      scale->scale.dynamic.max = 1.0 - scale->scale.dynamic.max;
      break;
  }
}

#define SCALE(name)                                                            \
  static int split_scale_##name(struct hikari_split_scale *scale,              \
      struct wlr_box *src,                                                     \
      struct hikari_view *first)                                               \
  {                                                                            \
    int gap = hikari_configuration->gap;                                       \
    struct wlr_box *geometry;                                                  \
    int name = 0;                                                              \
                                                                               \
    switch (scale->type) {                                                     \
      case HIKARI_SPLIT_SCALE_TYPE_FIXED:                                      \
        name =                                                                 \
            hikari_geometry_scale_fixed_##name(src, scale->scale.fixed, gap);  \
        break;                                                                 \
                                                                               \
      case HIKARI_SPLIT_SCALE_TYPE_DYNAMIC:                                    \
        geometry = hikari_view_geometry(first);                                \
        name = hikari_geometry_scale_dynamic_##name(src,                       \
            geometry,                                                          \
            scale->scale.dynamic.min,                                          \
            scale->scale.dynamic.max,                                          \
            gap);                                                              \
        break;                                                                 \
    }                                                                          \
                                                                               \
    assert(name != 0);                                                         \
                                                                               \
    return name;                                                               \
  }

SCALE(width)
SCALE(height)
#undef SCALE

static struct hikari_split *
copy_split(struct hikari_split *split);

#define COPY(name, first, second)                                              \
  static struct hikari_split *copy_split_##name(                               \
      struct hikari_split_##name *split_##name)                                \
  {                                                                            \
    struct hikari_split *first = copy_split(split_##name->first);              \
    struct hikari_split *second = copy_split(split_##name->second);            \
                                                                               \
    struct hikari_split_##name *ret =                                          \
        hikari_malloc(sizeof(struct hikari_split_##name));                     \
                                                                               \
    hikari_split_##name##_init(                                                \
        ret, &split_##name->scale, split_##name->orientation, first, second);  \
                                                                               \
    return (struct hikari_split *)ret;                                         \
  }

COPY(vertical, left, right)
COPY(horizontal, top, bottom)
#undef COPY

static struct hikari_split *
copy_split_container(struct hikari_split_container *split_container)
{
  struct hikari_split_container *ret =
      hikari_malloc(sizeof(struct hikari_split_container));

  hikari_split_container_init(
      ret, split_container->max, split_container->layout);

  return (struct hikari_split *)ret;
}

static struct hikari_split *
copy_split(struct hikari_split *split)
{
  struct hikari_split *ret;
  switch (split->type) {
    case HIKARI_SPLIT_TYPE_VERTICAL:
      ret = copy_split_vertical((struct hikari_split_vertical *)split);
      break;

    case HIKARI_SPLIT_TYPE_HORIZONTAL:
      ret = copy_split_horizontal((struct hikari_split_horizontal *)split);
      break;

    case HIKARI_SPLIT_TYPE_CONTAINER:
      ret = copy_split_container((struct hikari_split_container *)split);
      break;

    default:
      ret = NULL;
      break;
  }

  return ret;
}

struct hikari_split *
hikari_split_copy(struct hikari_split *split)
{
  return copy_split(split);
}

static struct hikari_view *
apply_split(struct hikari_split *split,
    struct wlr_box *geometry,
    struct hikari_view *first,
    bool *center)
{
  if (first == NULL) {
    return NULL;
  }

  struct hikari_view *view = first;
  switch (split->type) {
    case HIKARI_SPLIT_TYPE_VERTICAL: {
      struct wlr_box left, right;
      struct hikari_split_vertical *split_vertical =
          (struct hikari_split_vertical *)split;

      int width = split_scale_width(&split_vertical->scale, geometry, first);
      int gap = hikari_configuration->gap + hikari_configuration->border * 2;

      hikari_geometry_split_vertical(geometry, width, gap, &left, &right);

      switch (split_vertical->orientation) {
        case HIKARI_VERTICAL_SPLIT_ORIENTATION_LEFT:
          view = apply_split(split_vertical->left, &left, view, center);
          view = apply_split(split_vertical->right, &right, view, center);
          break;

        case HIKARI_VERTICAL_SPLIT_ORIENTATION_RIGHT:
          view = apply_split(split_vertical->right, &right, view, center);
          view = apply_split(split_vertical->left, &left, view, center);
          break;
      }
    } break;

    case HIKARI_SPLIT_TYPE_HORIZONTAL: {
      struct wlr_box top, bottom;
      struct hikari_split_horizontal *split_horizontal =
          (struct hikari_split_horizontal *)split;

      int height =
          split_scale_height(&split_horizontal->scale, geometry, first);
      int gap = hikari_configuration->gap + hikari_configuration->border * 2;

      hikari_geometry_split_horizontal(geometry, height, gap, &top, &bottom);

      switch (split_horizontal->orientation) {
        case HIKARI_HORIZONTAL_SPLIT_ORIENTATION_TOP:
          view = apply_split(split_horizontal->top, &top, view, center);
          view = apply_split(split_horizontal->bottom, &bottom, view, center);
          break;

        case HIKARI_HORIZONTAL_SPLIT_ORIENTATION_BOTTOM:
          view = apply_split(split_horizontal->bottom, &bottom, view, center);
          view = apply_split(split_horizontal->top, &top, view, center);
          break;
      }
    } break;

    case HIKARI_SPLIT_TYPE_CONTAINER: {
      struct hikari_split_container *container =
          (struct hikari_split_container *)split;
      container->geometry = *geometry;
      view = container->layout(
          view->sheet, view, geometry, container->max, center);
    } break;
  }

  return view;
}

void
hikari_split_apply(struct hikari_split *split,
    struct wlr_box *geometry,
    struct hikari_view *first)
{
  if (first == NULL) {
    return;
  }

  bool center = true;

  hikari_geometry_shrink(
      geometry, hikari_configuration->gap + hikari_configuration->border);

  apply_split(split, geometry, first, &center);
}

void
hikari_split_container_init(struct hikari_split_container *container,
    int nr_of_views,
    hikari_layout_func layout)
{
  container->split.type = HIKARI_SPLIT_TYPE_CONTAINER;
  container->max = nr_of_views;
  container->layout = layout;
  container->geometry = (struct wlr_box){ 0 };
}

void
hikari_split_vertical_init(struct hikari_split_vertical *split_vertical,
    struct hikari_split_scale *scale,
    enum hikari_split_vertical_orientation orientation,
    struct hikari_split *left,
    struct hikari_split *right)
{
  split_vertical->split.type = HIKARI_SPLIT_TYPE_VERTICAL;
  split_vertical->orientation = orientation;
  split_vertical->left = left;
  split_vertical->right = right;

  memcpy(&split_vertical->scale, scale, sizeof(struct hikari_split_scale));

  if (split_vertical->orientation == HIKARI_VERTICAL_SPLIT_ORIENTATION_RIGHT) {
    flip_scale(&split_vertical->scale);
  }
}

void
hikari_split_horizontal_init(struct hikari_split_horizontal *split_horizontal,
    struct hikari_split_scale *scale,
    enum hikari_split_horizontal_orientation orientation,
    struct hikari_split *top,
    struct hikari_split *bottom)
{
  split_horizontal->split.type = HIKARI_SPLIT_TYPE_HORIZONTAL;
  split_horizontal->orientation = orientation;
  split_horizontal->top = top;
  split_horizontal->bottom = bottom;

  memcpy(&split_horizontal->scale, scale, sizeof(struct hikari_split_scale));

  if (split_horizontal->orientation ==
      HIKARI_HORIZONTAL_SPLIT_ORIENTATION_BOTTOM) {
    flip_scale(&split_horizontal->scale);
  }
}

void
hikari_split_free(struct hikari_split *split)
{
  switch (split->type) {
    case HIKARI_SPLIT_TYPE_VERTICAL: {
      struct hikari_split_vertical *split_vertical =
          (struct hikari_split_vertical *)split;

      hikari_split_free(split_vertical->left);
      hikari_split_free(split_vertical->right);
      hikari_free(split_vertical);
    } break;

    case HIKARI_SPLIT_TYPE_HORIZONTAL: {
      struct hikari_split_horizontal *split_horizontal =
          (struct hikari_split_horizontal *)split;

      hikari_split_free(split_horizontal->top);
      hikari_split_free(split_horizontal->bottom);
      hikari_free(split_horizontal);
    } break;

    case HIKARI_SPLIT_TYPE_CONTAINER: {
      struct hikari_split_container *container =
          (struct hikari_split_container *)split;

      hikari_free(container);
    } break;
  }
}

struct hikari_split *
hikari_sheet_default_split(struct hikari_sheet *sheet)
{
  return hikari_configuration_lookup_layout(
      hikari_configuration, 48 + sheet->nr);
}
