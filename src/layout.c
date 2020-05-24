#include <hikari/layout.h>

#include <assert.h>

#include <hikari/split.h>
#include <hikari/tile.h>
#include <hikari/view.h>

void
hikari_layout_init(struct hikari_layout *layout,
    struct hikari_split *split,
    struct hikari_sheet *sheet)
{
  layout->split = hikari_split_copy(split);
  layout->sheet = sheet;
  wl_list_init(&layout->tiles);
}

void
hikari_layout_fini(struct hikari_layout *layout)
{
  hikari_split_free(layout->split);
}

#define CYCLE_LAYOUT(name, link)                                               \
  struct hikari_view *hikari_layout_##name##_view(                             \
      struct hikari_layout *layout)                                            \
  {                                                                            \
    struct wl_list *link = layout->tiles.link;                                 \
    struct hikari_tile *link##_tile;                                           \
                                                                               \
    do {                                                                       \
      if (link == &layout->tiles) {                                            \
        return NULL;                                                           \
      } else {                                                                 \
        link##_tile = wl_container_of(link, link##_tile, layout_tiles);        \
      }                                                                        \
      link = link->link;                                                       \
    } while (hikari_view_is_hidden(link##_tile->view));                        \
                                                                               \
    assert(link##_tile != NULL);                                               \
    assert(link##_tile->view != NULL);                                         \
    assert(!hikari_view_is_hidden(link##_tile->view));                         \
                                                                               \
    return link##_tile->view;                                                  \
  }

CYCLE_LAYOUT(first, next)
CYCLE_LAYOUT(last, prev)
#undef CYCLE_LAYOUT

void
hikari_layout_reset(struct hikari_layout *layout)
{
  assert(layout != NULL);

  struct hikari_sheet *sheet = layout->sheet;

  struct hikari_view *view;
  wl_list_for_each (view, &sheet->views, sheet_views) {
    if (hikari_view_is_tiled(view)) {
      hikari_view_reset_geometry(view);
    }
  }
}

static void
restack(struct hikari_layout *layout)
{
  assert(layout != NULL);

  struct hikari_tile *tile;
  wl_list_for_each_reverse (tile, &layout->tiles, layout_tiles) {
    struct hikari_view *view = tile->view;

    if (hikari_view_is_hidden(view)) {
      hikari_view_show(view);
    } else {
      hikari_view_raise(view);
    }
  }
}

void
hikari_layout_restack_append(struct hikari_layout *layout)
{
  assert(layout != NULL);

  restack(layout);

  hikari_sheet_apply_split(layout->sheet, layout->split);
}

static bool
view_is_prependable(struct hikari_view *view)
{
  return !hikari_view_is_tiled(view) && hikari_view_is_tileable(view);
}

static void
raise_prependable(struct hikari_sheet *sheet)
{
  struct hikari_view *view, *view_temp, *first = NULL;
  wl_list_for_each_reverse_safe (view, view_temp, &sheet->views, sheet_views) {
    if (view_is_prependable(view)) {
      if (view == first) {
        break;
      } else if (first == NULL) {
        first = view;
      }

      hikari_view_raise(view);
    }
  }
}

void
hikari_layout_restack_prepend(struct hikari_layout *layout)
{
  assert(layout != NULL);

  restack(layout);
  raise_prependable(layout->sheet);

  hikari_sheet_apply_split(layout->sheet, layout->split);
}
