#if !defined(HIKARI_INDICATOR_H)
#define HIKARI_INDICATOR_H

#include <hikari/font.h>
#include <hikari/indicator_bar.h>
#include <hikari/sheet.h>

struct hikari_render_data;
struct hikari_view;
struct hikari_output;

struct hikari_indicator {
  struct hikari_indicator_bar title;
  struct hikari_indicator_bar sheet;
  struct hikari_indicator_bar group;
  struct hikari_indicator_bar mark;

  int bar_height;
};

void
hikari_indicator_init(
    struct hikari_indicator *indicator, float color[static 4]);

void
hikari_indicator_fini(struct hikari_indicator *indicator);

void
hikari_indicator_render(
    struct hikari_indicator *indicator, struct hikari_render_data *render_data);

void
hikari_indicator_update(struct hikari_indicator *indicator,
    struct hikari_view *view,
    float background[static 4]);

void
hikari_indicator_update_sheet(struct hikari_indicator *indicator,
    struct wlr_box *view_geometry,
    struct hikari_output *output,
    struct hikari_sheet *sheet,
    float background[static 4],
    bool invisible,
    bool floating);

void
hikari_indicator_damage(
    struct hikari_indicator *indicator, struct hikari_view *view);

static inline void
hikari_indicator_update_title(struct hikari_indicator *indicator,
    struct wlr_box *view_geometry,
    struct hikari_output *output,
    const char *text,
    float background[static 4])
{
  hikari_indicator_bar_update(
      &indicator->title, view_geometry, output, text, background);
}

static inline void
hikari_indicator_update_group(struct hikari_indicator *indicator,
    struct wlr_box *view_geometry,
    struct hikari_output *output,
    const char *text,
    float background[static 4])
{
  hikari_indicator_bar_update(
      &indicator->group, view_geometry, output, text, background);
}

static inline void
hikari_indicator_update_mark(struct hikari_indicator *indicator,
    struct wlr_box *view_geometry,
    struct hikari_output *output,
    const char *text,
    float background[static 4])
{
  hikari_indicator_bar_update(
      &indicator->mark, view_geometry, output, text, background);
}

#endif
