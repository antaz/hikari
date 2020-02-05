#if !defined(HIKARI_OUTPUT_H)
#define HIKARI_OUTPUT_H

#include <assert.h>

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_surface.h>

struct hikari_output {
  struct wl_list link;
  struct wlr_output *output;
  struct wlr_output_damage *damage;
  struct hikari_workspace *workspace;

  struct wl_listener damage_frame;
  /* struct wl_listener mode; */
  struct wl_listener destroy;

  struct wl_list views;
#ifdef HAVE_XWAYLAND
  struct wl_list unmanaged_xwayland_views;
#endif
  struct wl_list server_outputs;

  struct wlr_box geometry;
};

void
hikari_output_init(struct hikari_output *output, struct wlr_output *wlr_output);

void
hikari_output_fini(struct hikari_output *output);

void
hikari_output_scissor_render(struct wlr_output *wlr_output,
    struct wlr_renderer *renderer,
    pixman_box32_t *rect);

void
hikari_output_damage_whole(struct hikari_output *output);

void
hikari_output_disable(struct hikari_output *output);

void
hikari_output_enable(struct hikari_output *output);

static inline void
hikari_output_add_damage(struct hikari_output *output, struct wlr_box *region)
{
  assert(output != NULL);
  assert(region != NULL);

  wlr_output_damage_add_box(output->damage, region);
}

#endif
