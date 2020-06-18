#if !defined(HIKARI_RENDER_H)
#define HIKARI_RENDER_H

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/region.h>

struct hikari_output;
struct hikari_render_data;

void
damage_frame_handler(struct wl_listener *listener, void *);

void
damage_empty_frame_handler(struct wl_listener *listener, void *);

void
hikari_render_visible_views(
    struct hikari_output *output, struct hikari_render_data *render_data);

void
hikari_render_background(struct hikari_output *output,
    struct hikari_render_data *render_data,
    float alpha);

void
hikari_render_texture(struct wlr_texture *texture,
    struct wlr_output *output,
    pixman_region32_t *damage,
    struct wlr_renderer *renderer,
    const float matrix[static 9],
    struct wlr_box *box,
    float alpha);

#endif
