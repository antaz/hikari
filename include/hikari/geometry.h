#if !defined(HIKARI_GEOMETRY_H)
#define HIKARI_GEOMETRY_H

#include <wlr/types/wlr_box.h>

void
hikari_geometry_split_vertical(struct wlr_box *src,
    float factor,
    int gap,
    struct wlr_box *left,
    struct wlr_box *right);

void
hikari_geometry_split_horizontal(struct wlr_box *src,
    float factor,
    int gap,
    struct wlr_box *top,
    struct wlr_box *bottom);

void
hikari_geometry_shrink(struct wlr_box *geometry, int gap);

void
hikari_geometry_constrain_position(struct wlr_box *geometry,
    int screen_width,
    int screen_height,
    int x,
    int y);

#endif
