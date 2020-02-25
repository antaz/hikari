#if !defined(HIKARI_TILE_H)
#define HIKARI_TILE_H

#include <wlr/types/wlr_box.h>

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

void
hikari_tile_fini(struct hikari_tile *tile);

struct hikari_view *
hikari_tile_next_view(struct hikari_tile *tile);

struct hikari_view *
hikari_tile_prev_view(struct hikari_tile *tile);

#endif
