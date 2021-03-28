#if !defined(HIKARI_RENDER_H)
#define HIKARI_RENDER_H

#include <time.h>

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output.h>

struct hikari_output;

struct hikari_renderer {
  struct wlr_output *wlr_output;
  struct wlr_renderer *wlr_renderer;
  pixman_region32_t *damage;
  struct wlr_box *geometry;
};

void
hikari_renderer_damage_frame_handler(struct wl_listener *listener, void *);

void
hikari_renderer_normal_mode(struct hikari_renderer *renderer);

void
hikari_renderer_group_assign_mode(struct hikari_renderer *renderer);

void
hikari_renderer_input_grab_mode(struct hikari_renderer *renderer);

void
hikari_renderer_lock_mode(struct hikari_renderer *renderer);

void
hikari_renderer_mark_assign_mode(struct hikari_renderer *renderer);

void
hikari_renderer_mark_select_mode(struct hikari_renderer *renderer);

void
hikari_renderer_move_mode(struct hikari_renderer *renderer);

void
hikari_renderer_resize_mode(struct hikari_renderer *renderer);

void
hikari_renderer_sheet_assign_mode(struct hikari_renderer *renderer);

void
hikari_renderer_dnd_mode(struct hikari_renderer *renderer);

void
hikari_renderer_layout_select_mode(struct hikari_renderer *renderer);

#endif
