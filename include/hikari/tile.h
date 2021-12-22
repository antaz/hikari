#if !defined(HIKARI_TILE_H)
#define HIKARI_TILE_H

#include <wlr/util/box.h>

struct hikari_layout;
struct hikari_view;

struct hikari_tile {
  struct hikari_view *view;
  struct hikari_layout *layout;
  struct wlr_box view_geometry;
  struct wlr_box tile_geometry;

  struct wl_list layout_tiles;
};

void
hikari_tile_init(struct hikari_tile *tile,
    struct hikari_view *view,
    struct hikari_layout *layout,
    struct wlr_box *tile_geometry,
    struct wlr_box *view_geometry);

struct hikari_view *
hikari_tile_next_view(struct hikari_tile *tile);

struct hikari_view *
hikari_tile_prev_view(struct hikari_tile *tile);

void
hikari_tile_detach(struct hikari_tile *tile);

static inline bool
hikari_tile_is_attached(struct hikari_tile *tile)
{
  return tile->layout != NULL;
}

#endif
