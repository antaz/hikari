#include <hikari/split.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/geometry.h>
#include <hikari/render_data.h>
#include <hikari/view.h>

static struct hikari_view *
apply_split(struct hikari_split *split,
    struct wlr_box *geometry,
    struct hikari_view *first)
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

      switch (split_vertical->orientation) {
        case HIKARI_VERTICAL_SPLIT_ORIENTATION_LEFT:
          hikari_geometry_split_vertical(geometry,
              split_vertical->scale,
              hikari_configuration->gap + hikari_configuration->border * 2,
              &left,
              &right);

          view = apply_split(split_vertical->left, &left, view);
          view = apply_split(split_vertical->right, &right, view);
          break;

        case HIKARI_VERTICAL_SPLIT_ORIENTATION_RIGHT:
          hikari_geometry_split_vertical(geometry,
              1.0 - split_vertical->scale,
              hikari_configuration->gap + hikari_configuration->border * 2,
              &left,
              &right);

          view = apply_split(split_vertical->right, &right, view);
          view = apply_split(split_vertical->left, &left, view);
          break;

        default:
          break;
      }
    } break;

    case HIKARI_SPLIT_TYPE_HORIZONTAL: {
      struct wlr_box top, bottom;
      struct hikari_split_horizontal *split_horizontal =
          (struct hikari_split_horizontal *)split;

      switch (split_horizontal->orientation) {
        case HIKARI_HORIZONTAL_SPLIT_ORIENTATION_TOP:
          hikari_geometry_split_horizontal(geometry,
              split_horizontal->scale,
              hikari_configuration->gap + hikari_configuration->border * 2,
              &top,
              &bottom);

          view = apply_split(split_horizontal->top, &top, view);
          view = apply_split(split_horizontal->bottom, &bottom, view);
          break;

        case HIKARI_HORIZONTAL_SPLIT_ORIENTATION_BOTTOM:
          hikari_geometry_split_horizontal(geometry,
              1.0 - split_horizontal->scale,
              hikari_configuration->gap + hikari_configuration->border * 2,
              &top,
              &bottom);

          view = apply_split(split_horizontal->bottom, &bottom, view);
          view = apply_split(split_horizontal->top, &top, view);
          break;

        default:
          break;
      }
    } break;

    case HIKARI_SPLIT_TYPE_CONTAINER: {
      struct hikari_split_container *container =
          (struct hikari_split_container *)split;
      container->geometry = *geometry;
      view = container->layout(view->sheet, view, geometry, container->max);
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

  hikari_geometry_shrink(
      geometry, hikari_configuration->gap + hikari_configuration->border);

  apply_split(split, geometry, first);
}

void
hikari_split_container_init(struct hikari_split_container *container,
    int nr_of_views,
    layout_func_t layout)
{
  container->split.type = HIKARI_SPLIT_TYPE_CONTAINER;
  container->max = nr_of_views;
  container->layout = layout;
  container->geometry = (struct wlr_box){ 0 };
}

void
hikari_split_vertical_init(struct hikari_split_vertical *split_vertical,
    float scale,
    enum hikari_split_vertical_orientation orientation,
    struct hikari_split *left,
    struct hikari_split *right)
{
  split_vertical->split.type = HIKARI_SPLIT_TYPE_VERTICAL;
  split_vertical->scale = scale;
  split_vertical->orientation = orientation;
  split_vertical->left = left;
  split_vertical->right = right;
}

void
hikari_split_horizontal_init(struct hikari_split_horizontal *split_horizontal,
    float scale,
    enum hikari_split_horizontal_orientation orientation,
    struct hikari_split *top,
    struct hikari_split *bottom)
{
  split_horizontal->split.type = HIKARI_SPLIT_TYPE_HORIZONTAL;
  split_horizontal->scale = scale;
  split_horizontal->orientation = orientation;
  split_horizontal->top = top;
  split_horizontal->bottom = bottom;
}

void
hikari_split_fini(struct hikari_split *split)
{
  switch (split->type) {
    case HIKARI_SPLIT_TYPE_VERTICAL: {
      struct hikari_split_vertical *split_vertical =
          (struct hikari_split_vertical *)split;

      hikari_split_fini(split_vertical->left);
      hikari_split_fini(split_vertical->right);
      hikari_free(split_vertical);
    } break;

    case HIKARI_SPLIT_TYPE_HORIZONTAL: {
      struct hikari_split_horizontal *split_horizontal =
          (struct hikari_split_horizontal *)split;

      hikari_split_fini(split_horizontal->top);
      hikari_split_fini(split_horizontal->bottom);
      hikari_free(split_horizontal);
    } break;

    case HIKARI_SPLIT_TYPE_CONTAINER: {
      struct hikari_split_container *container =
          (struct hikari_split_container *)split;

      hikari_free(container);
    } break;
  }
}
