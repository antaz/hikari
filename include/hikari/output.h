#if !defined(HIKARI_OUTPUT_H)
#define HIKARI_OUTPUT_H

#include <assert.h>

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_surface.h>

#include <hikari/output_config.h>

struct hikari_render_data;

struct hikari_output {
  struct wlr_output *wlr_output;
  struct wlr_output_damage *damage;
  struct hikari_workspace *workspace;

  bool enabled;

  struct wl_listener damage_frame;
  struct wl_listener destroy;
  struct wl_listener damage_destroy;
  /* struct wl_listener mode; */

#ifdef HAVE_LAYERSHELL
  struct wl_list layers[4];
#endif

  struct wl_list views;
#ifdef HAVE_XWAYLAND
  struct wl_list unmanaged_xwayland_views;
#endif
  struct wl_list server_outputs;

  struct wlr_box geometry;
  struct wlr_box usable_area;

  struct wlr_texture *background;
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

void
hikari_output_enable_content(struct hikari_output *output);

void
hikari_output_disable_content(struct hikari_output *output);

void
hikari_output_load_background(struct hikari_output *output,
    const char *path,
    enum hikari_background_fit background_fit);

void
hikari_output_render_background(struct hikari_output *output,
    struct hikari_render_data *render_data,
    float alpha);

void
hikari_output_render_visible_views(
    struct hikari_output *output, struct hikari_render_data *render_data);

void
hikari_output_move(struct hikari_output *output, double lx, double ly);

static inline void
hikari_output_add_damage(struct hikari_output *output, struct wlr_box *region)
{
  assert(output != NULL);
  assert(region != NULL);

  if (output->enabled) {
    wlr_output_damage_add_box(output->damage, region);
  }
}

#endif
