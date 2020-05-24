#include <hikari/tile.h>

#include <wayland-util.h>

#include <hikari/layout.h>
#include <hikari/sheet.h>
#include <hikari/view.h>

void
hikari_tile_init(struct hikari_tile *tile,
    struct hikari_view *view,
    struct hikari_layout *layout,
    struct wlr_box *tile_geometry,
    struct wlr_box *view_geometry)
{
  tile->view = view;
  tile->layout = layout;
  tile->tile_geometry = *tile_geometry;
  tile->view_geometry = *view_geometry;
}

#define CYCLE_LAYOUT(link)                                                     \
  struct hikari_view *hikari_tile_##link##_view(struct hikari_tile *tile)      \
  {                                                                            \
    assert(!hikari_view_is_hidden(tile->view));                                \
                                                                               \
    struct wl_list *link = tile->layout_tiles.link;                            \
    struct hikari_tile *link##_tile;                                           \
    struct wl_list *tiles = &hikari_server.workspace->sheet->layout->tiles;    \
                                                                               \
    do {                                                                       \
      if (link == tiles) {                                                     \
        link##_tile = wl_container_of(tiles->link, link##_tile, layout_tiles); \
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

CYCLE_LAYOUT(next)
CYCLE_LAYOUT(prev)
#undef CYCLE_LAYOUT

void
hikari_tile_detach(struct hikari_tile *tile)
{
  struct hikari_layout *layout = tile->layout;

  if (layout != NULL) {
    wl_list_remove(&tile->layout_tiles);

    if (wl_list_empty(&layout->tiles)) {
      layout->sheet->layout = NULL;
      hikari_layout_fini(layout);
      hikari_free(layout);
    }
    tile->layout = NULL;
  }
}
