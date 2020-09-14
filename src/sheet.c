#include <hikari/sheet.h>

#include <stdio.h>
#include <stdlib.h>

#include <hikari/configuration.h>
#include <hikari/group.h>
#include <hikari/layout.h>
#include <hikari/memory.h>
#include <hikari/split.h>
#include <hikari/view.h>

void
hikari_sheet_init(
    struct hikari_sheet *sheet, int nr, struct hikari_workspace *workspace)
{
  assert(workspace->output != NULL);

  wl_list_init(&sheet->views);

  sheet->nr = nr;

  sheet->workspace = workspace;
  sheet->layout = NULL;
}

static struct hikari_view *
scan_next_tileable_view(struct hikari_view *view)
{
  assert(view != NULL);

  struct wl_list *next = view->sheet_views.next;

  while (next != &view->sheet->views) {
    view = wl_container_of(next, view, sheet_views);
    if (hikari_view_is_tileable(view)) {
      assert(!hikari_view_is_hidden(view));
      return view;
    }
    next = view->sheet_views.next;
  }

  return NULL;
}

struct hikari_view *
hikari_sheet_first_tileable_view(struct hikari_sheet *sheet)
{
  assert(sheet != NULL);

  struct wl_list *next = sheet->views.next;
  struct hikari_view *view = NULL;

  while (next != &sheet->views) {
    view = wl_container_of(next, view, sheet_views);
    if (hikari_view_is_tileable(view)) {
      if (hikari_view_is_hidden(view)) {
        hikari_view_show(view);
      }
      return view;
    }
    next = view->sheet_views.next;
  }

  return NULL;
}

static int
tileable_views(struct hikari_view *view)
{
  int result;
  if (hikari_view_is_tileable(view)) {
    result = 1;
  } else {
    result = 0;
  }

  struct wl_list *next = view->sheet_views.next;

  while (next != &view->sheet->views) {
    view = wl_container_of(next, view, sheet_views);
    if (hikari_view_is_tileable(view)) {
      result++;
    }
    next = view->sheet_views.next;
  }

  return result;
}

static struct hikari_view *
single_layout(struct wlr_box *frame,
    struct hikari_view *first,
    int nr_of_views,
    bool *center)
{
  hikari_view_tile(first, frame, *center);
  *center = false;
  return scan_next_tileable_view(first);
}

static struct hikari_view *
empty_layout(struct wlr_box *frame,
    struct hikari_view *first,
    int nr_of_views,
    bool *center)
{
  return first;
}

static struct hikari_view *
full_layout(struct wlr_box *frame,
    struct hikari_view *first,
    int nr_of_views,
    bool *center)
{
  struct hikari_view *view = first;
  for (int i = 0; i < nr_of_views && view != NULL; i++) {
    if (hikari_view_is_tileable(view)) {
      if (hikari_view_is_hidden(view)) {
        hikari_view_show(view);
      }
      hikari_view_tile(view, frame, *center);
      *center = false;
    }
    view = scan_next_tileable_view(view);
  }

  return view;
}

#define LAYOUT_VIEWS(nr_of_views, view, frame, center)                         \
  if (nr_of_views == 0) {                                                      \
    return NULL;                                                               \
  } else if (nr_of_views == 1) {                                               \
    hikari_view_tile(view, frame, *center);                                    \
    *center = false;                                                           \
  } else

static struct hikari_view *
grid_layout(struct wlr_box *frame,
    struct hikari_view *first,
    int nr_of_views,
    bool *center)
{
  int nr_of_rows = 1;
  int nr_of_cols = 1;

  for (int i = 1; i <= nr_of_views; i++) {
    if (i > nr_of_rows * nr_of_cols) {
      if (nr_of_cols > nr_of_rows) {
        assert(nr_of_cols == nr_of_rows + 1);
        nr_of_rows++;
      } else {
        nr_of_cols++;
      }
    }
  }

  struct hikari_view *view = first;
  LAYOUT_VIEWS(nr_of_views, first, frame, center)
  {
    int border_width = hikari_configuration->border;
    int gap = hikari_configuration->gap;
    int border = 2 * border_width;
    int row_gaps = nr_of_rows - 1;
    int col_gaps = nr_of_cols - 1;
    int gaps_height = gap * row_gaps;
    int gaps_width = gap * col_gaps;
    int views_height = frame->height - border * nr_of_rows - gaps_height;
    int views_width = frame->width - border * nr_of_cols - gaps_width;

    int width = views_width / nr_of_cols;
    int height = views_height / nr_of_rows;

    int rest_width =
        frame->width - border * col_gaps - gaps_width - width * nr_of_cols;

    int rest_height =
        frame->height - border * row_gaps - gaps_height - height * nr_of_rows;

    struct wlr_box geometry = { .y = frame->y, .x = frame->x };

    geometry.height = height + rest_height;
    for (int g_y = 0; g_y < nr_of_rows; g_y++) {
      if (g_y == 1) {
        geometry.height = height;
      }
      geometry.width = width + rest_width;
      for (int g_x = 0; g_x < nr_of_cols; g_x++) {
        if (g_x == 1) {
          geometry.width = width;
        }
        hikari_view_tile(view, &geometry, *center);
        *center = false;

        view = scan_next_tileable_view(view);
        if (view == NULL) {
          return NULL;
        }

        geometry.x += gap + border + geometry.width;
      }
      geometry.x = frame->x;
      geometry.y += gap + border + geometry.height;
    }
  }

  return scan_next_tileable_view(view);
}

#define SPLIT_LAYOUT(name, x, y, width, height)                                \
  static struct hikari_view *name##_layout(struct wlr_box *frame,              \
      struct hikari_view *first,                                               \
      int nr_of_views,                                                         \
      bool *center)                                                            \
  {                                                                            \
    struct hikari_view *view = first;                                          \
    int border_width = hikari_configuration->border;                           \
    int gap = hikari_configuration->gap;                                       \
    int border = 2 * border_width;                                             \
    int gaps = nr_of_views - 1;                                                \
    int gaps_##width = gap * gaps;                                             \
                                                                               \
    LAYOUT_VIEWS(nr_of_views, first, frame, center)                            \
    {                                                                          \
      int views_width = frame->width - border * gaps - gaps_##width;           \
      int width = views_width / nr_of_views;                                   \
      int rest = views_width - width * nr_of_views;                            \
                                                                               \
      struct wlr_box geometry = { .x = frame->x,                               \
        .y = frame->y,                                                         \
        .width = width + rest,                                                 \
        .height = frame->height };                                             \
                                                                               \
      hikari_view_tile(first, &geometry, *center);                             \
      *center = false;                                                         \
                                                                               \
      geometry.x += gap + border + width + rest;                               \
      geometry.width = width;                                                  \
      for (int n = 1; n < nr_of_views; n++) {                                  \
        view = scan_next_tileable_view(view);                                  \
        hikari_view_tile(view, &geometry, *center);                            \
        *center = false;                                                       \
        geometry.x += gap + border + width;                                    \
      }                                                                        \
    }                                                                          \
                                                                               \
    return scan_next_tileable_view(view);                                      \
  }

SPLIT_LAYOUT(queue, x, y, width, height)
SPLIT_LAYOUT(stack, y, x, height, width)
#undef SPLIT_LAYOUT

#define LAYOUT(name)                                                           \
  struct hikari_view *hikari_sheet_##name##_layout(struct hikari_sheet *sheet, \
      struct hikari_view *first,                                               \
      struct wlr_box *frame,                                                   \
      int max,                                                                 \
      bool *center)                                                            \
  {                                                                            \
    int nr_of_views = tileable_views(first);                                   \
    if (nr_of_views > max) {                                                   \
      nr_of_views = max;                                                       \
    }                                                                          \
    if (nr_of_views == 0 && max != 0) {                                        \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    return name##_layout(frame, first, nr_of_views, center);                   \
  }

LAYOUT(queue)
LAYOUT(stack)
LAYOUT(grid)
LAYOUT(full)
LAYOUT(single)
LAYOUT(empty)
#undef LAYOUT

#undef LAYOUT_WINDOWS

#define SHEET_VIEW(name, link)                                                 \
  struct hikari_view *hikari_sheet_##name##_view(struct hikari_sheet *sheet)   \
  {                                                                            \
    struct wl_list *link = sheet->views.link;                                  \
    struct hikari_view *view;                                                  \
                                                                               \
    while (link != &sheet->views) {                                            \
      view = wl_container_of(link, view, sheet_views);                         \
      if (!hikari_view_is_hidden(view)) {                                      \
        return view;                                                           \
      }                                                                        \
      link = view->sheet_views.link;                                           \
    }                                                                          \
                                                                               \
    return NULL;                                                               \
  }

SHEET_VIEW(first, next)
SHEET_VIEW(last, prev)
#undef SHEET_VIEW

#define SHEET_VIEW(link, fallback)                                             \
  struct hikari_view *hikari_sheet_##link##_view(                              \
      struct hikari_sheet *sheet, struct hikari_view *view)                    \
  {                                                                            \
    struct wl_list *link = view->sheet_views.link;                             \
                                                                               \
    while (link != &view->sheet->views) {                                      \
      view = wl_container_of(link, view, sheet_views);                         \
      if (!hikari_view_is_hidden(view)) {                                      \
        return view;                                                           \
      }                                                                        \
      link = view->sheet_views.link;                                           \
    }                                                                          \
                                                                               \
    return hikari_sheet_##fallback##_view(sheet);                              \
  }

SHEET_VIEW(next, first)
SHEET_VIEW(prev, last)
#undef SHEET_VIEW

int
hikari_sheet_tileable_views(struct hikari_sheet *sheet)
{
  int nr_of_views = 0;

  struct hikari_view *view;
  wl_list_for_each (view, &sheet->views, sheet_views) {
    if (hikari_view_is_tileable(view)) {
      nr_of_views++;
    }
  }

  return nr_of_views;
}

struct hikari_sheet *
hikari_sheet_next(struct hikari_sheet *sheet)
{
  struct hikari_sheet *sheets = sheet->workspace->sheets;

  if (sheet->nr == 9) {
    return &sheets[1];
  }

  return &sheets[sheet->nr + 1];
}

struct hikari_sheet *
hikari_sheet_prev(struct hikari_sheet *sheet)
{
  struct hikari_sheet *sheets = sheet->workspace->sheets;

  if (sheet->nr == 0 || sheet->nr == 1) {
    return &sheets[9];
  }

  return &sheets[sheet->nr - 1];
}

#define CYCLE_INHABITED(name)                                                  \
  struct hikari_sheet *hikari_sheet_##name##_inhabited(                        \
      struct hikari_sheet *sheet)                                              \
  {                                                                            \
    struct hikari_sheet *name = sheet;                                         \
                                                                               \
    do {                                                                       \
      name = hikari_sheet_##name(name);                                        \
    } while (wl_list_empty(&name->views) && name != sheet);                    \
                                                                               \
    return name;                                                               \
  }

CYCLE_INHABITED(next)
CYCLE_INHABITED(prev)
#undef CYCLE_INHABITED

static void
raise_floating(struct hikari_sheet *sheet)
{
  struct hikari_view *view, *view_temp, *first = NULL;
  wl_list_for_each_reverse_safe (view, view_temp, &sheet->views, sheet_views) {
    if (!hikari_view_is_hidden(view) && hikari_view_is_floating(view)) {
      if (first == view) {
        break;
      } else if (first == NULL) {
        first = view;
      }

      hikari_view_raise(view);
    }
  }
}

void
hikari_sheet_apply_split(struct hikari_sheet *sheet, struct hikari_split *split)
{
  struct hikari_layout *layout;
  if (sheet->layout != NULL) {
    layout = sheet->layout;

    struct hikari_tile *tile;
    wl_list_for_each (tile, &layout->tiles, layout_tiles) {
      if (hikari_view_is_dirty(tile->view)) {
        return;
      }
    }

    if (layout->split != split) {
      hikari_split_free(layout->split);
      layout->split = hikari_split_copy(split);
    }
  } else {
    layout = hikari_malloc(sizeof(struct hikari_layout));
    hikari_layout_init(layout, split, sheet);

    sheet->layout = layout;
  }

  struct hikari_output *output = sheet->workspace->output;
  struct wlr_box geometry = output->usable_area;
  struct hikari_view *first = hikari_sheet_first_tileable_view(sheet);

  hikari_split_apply(layout->split, &geometry, first);

  raise_floating(sheet);
}

bool
hikari_sheet_is_visible(struct hikari_sheet *sheet)
{
  struct hikari_sheet *sheets = sheet->workspace->sheets;

  return sheet == sheet->workspace->sheet || sheet == &sheets[0];
}

#define SHOW_VIEWS(cond)                                                       \
  {                                                                            \
    struct hikari_view *view, *view_temp;                                      \
    wl_list_for_each_reverse_safe (                                            \
        view, view_temp, &sheet->views, sheet_views) {                         \
      if (cond) {                                                              \
        if (hikari_view_is_hidden(view)) {                                     \
          hikari_view_show(view);                                              \
        } else {                                                               \
          break;                                                               \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

void
hikari_sheet_show_all(struct hikari_sheet *sheet)
{
  SHOW_VIEWS(true);
}

void
hikari_sheet_show(struct hikari_sheet *sheet)
{
  SHOW_VIEWS(!hikari_view_is_invisible(view));
}

void
hikari_sheet_show_group(struct hikari_sheet *sheet, struct hikari_group *group)
{
  SHOW_VIEWS(view->group == group);
}

void
hikari_sheet_show_invisible(struct hikari_sheet *sheet)
{
  SHOW_VIEWS(hikari_view_is_invisible(view));
}
#undef SHOW_VIEWS
