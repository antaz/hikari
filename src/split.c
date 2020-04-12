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
      struct hikari_container *container = (struct hikari_container *)split;
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
hikari_container_init(
    struct hikari_container *container, int nr_of_views, layout_func_t layout)
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

static void
rect_render(float color[static 4],
    struct wlr_box *box,
    struct hikari_render_data *render_data)
{
  pixman_region32_t damage;
  pixman_region32_init(&damage);
  pixman_region32_union_rect(
      &damage, &damage, box->x, box->y, box->width, box->height);

  pixman_region32_intersect(&damage, &damage, render_data->damage);
  bool damaged = pixman_region32_not_empty(&damage);
  if (!damaged) {
    goto buffer_damage_finish;
  }

  struct wlr_renderer *renderer = render_data->renderer;
  assert(renderer);

  float matrix[9];
  wlr_matrix_project_box(matrix,
      box,
      WL_OUTPUT_TRANSFORM_NORMAL,
      0,
      render_data->output->transform_matrix);

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; i++) {
    hikari_output_scissor_render(render_data->output, renderer, &rects[i]);
    wlr_render_quad_with_matrix(renderer, color, matrix);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
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
      struct hikari_container *container = (struct hikari_container *)split;

      hikari_free(container);
    } break;
  }
}

void
hikari_split_render(
    struct hikari_split *split, struct hikari_render_data *render_data)
{
  switch (split->type) {
    case HIKARI_SPLIT_TYPE_VERTICAL: {
      struct hikari_split_vertical *split_vertical =
          (struct hikari_split_vertical *)split;

      hikari_split_render(split_vertical->left, render_data);
      hikari_split_render(split_vertical->right, render_data);
    } break;

    case HIKARI_SPLIT_TYPE_HORIZONTAL: {
      struct hikari_split_horizontal *split_horizontal =
          (struct hikari_split_horizontal *)split;

      hikari_split_render(split_horizontal->top, render_data);
      hikari_split_render(split_horizontal->bottom, render_data);
    } break;

    case HIKARI_SPLIT_TYPE_CONTAINER: {
      struct hikari_container *container = (struct hikari_container *)split;

      hikari_container_render(container, render_data);
    } break;
  }
}

void
hikari_container_render(
    struct hikari_container *container, struct hikari_render_data *render_data)
{
  float color[4];
  hikari_color_convert(color, 0xAE81FF);
  color[3] = 0.5;

  render_data->geometry = &container->geometry;

  rect_render(color, &container->geometry, render_data);
}
