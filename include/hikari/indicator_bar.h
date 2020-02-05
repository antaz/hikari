#if !defined(HIKARI_INDICATOR_BAR_H)
#define HIKARI_INDICATOR_BAR_H

#include <wlr/types/wlr_surface.h>

struct hikari_indicator;
struct hikari_render_data;
struct hikari_output;

struct hikari_indicator_bar {
  struct wlr_texture *texture;
  struct hikari_indicator *indicator;

  int width;
  int offset;
};

void
hikari_indicator_bar_init(struct hikari_indicator_bar *indicator_bar,
    struct hikari_indicator *indicator,
    int offset);

void
hikari_indicator_bar_fini(struct hikari_indicator_bar *indicator_bar);

void
hikari_indicator_bar_update(struct hikari_indicator_bar *indicator_bar,
    struct wlr_box *view_geometry,
    struct hikari_output *output,
    const char *text,
    float background[static 4]);

void
hikari_indicator_bar_render(struct hikari_indicator_bar *indicator_bar,
    struct hikari_render_data *render_data);

void
hikari_indicator_bar_damage(struct hikari_indicator_bar *indicator_bar,
    struct wlr_box *view_geometry,
    struct hikari_output *output);

#endif
