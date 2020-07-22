#if !defined(HIKARI_INDICATOR_BAR_H)
#define HIKARI_INDICATOR_BAR_H

#include <wlr/types/wlr_surface.h>

struct hikari_indicator;
struct hikari_renderer;
struct hikari_output;

struct hikari_indicator_bar {
  struct wlr_texture *texture;
  struct hikari_indicator *indicator;

  int width;
  int offset;

  float color[4];
};

void
hikari_indicator_bar_init(struct hikari_indicator_bar *indicator_bar,
    struct hikari_indicator *indicator,
    int offset,
    float color[static 4]);

void
hikari_indicator_bar_fini(struct hikari_indicator_bar *indicator_bar);

void
hikari_indicator_bar_set_color(
    struct hikari_indicator_bar *indicator_bar, float color[static 4]);

void
hikari_indicator_bar_update(struct hikari_indicator_bar *indicator_bar,
    struct hikari_output *output,
    const char *text);

void
hikari_indicator_bar_damage(struct hikari_indicator_bar *indicator_bar,
    struct hikari_output *output,
    struct wlr_box *view_geometry);

#endif
