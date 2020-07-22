#if !defined(HIKARI_INDICATOR_H)
#define HIKARI_INDICATOR_H

#include <hikari/font.h>
#include <hikari/indicator_bar.h>
#include <hikari/sheet.h>

struct hikari_renderer;
struct hikari_view;
struct hikari_output;

struct hikari_indicator {
  struct hikari_indicator_bar title;
  struct hikari_indicator_bar sheet;
  struct hikari_indicator_bar group;
  struct hikari_indicator_bar mark;
};

void
hikari_indicator_init(
    struct hikari_indicator *indicator, float color[static 4]);

void
hikari_indicator_fini(struct hikari_indicator *indicator);

void
hikari_indicator_update(
    struct hikari_indicator *indicator, struct hikari_view *view);

void
hikari_indicator_set_color(
    struct hikari_indicator *indicator, float color[static 4]);

void
hikari_indicator_update_sheet(struct hikari_indicator *indicator,
    struct hikari_output *output,
    struct hikari_sheet *sheet,
    unsigned long flags);

void
hikari_indicator_damage(
    struct hikari_indicator *indicator, struct hikari_view *view);

#define UPDATE(name)                                                           \
  static inline void hikari_indicator_update_##name(                           \
      struct hikari_indicator *indicator,                                      \
      struct hikari_output *output,                                            \
      const char *text)                                                        \
  {                                                                            \
    hikari_indicator_bar_update(&indicator->name, output, text);               \
  }

UPDATE(title)
UPDATE(group)
UPDATE(mark)
#undef UPDATE

#define DAMAGE(name)                                                           \
  static inline void hikari_indicator_damage_##name(                           \
      struct hikari_indicator *indicator,                                      \
      struct hikari_output *output,                                            \
      struct wlr_box *geometry)                                                \
  {                                                                            \
    hikari_indicator_bar_damage(&indicator->name, output, geometry);           \
  }

DAMAGE(title)
DAMAGE(sheet)
DAMAGE(group)
DAMAGE(mark)
#undef DAMAGE

#define COLOR(name)                                                            \
  static inline void hikari_indicator_set_color_##name(                        \
      struct hikari_indicator *indicator, float color[static 4])               \
  {                                                                            \
    hikari_indicator_bar_set_color(&indicator->name, color);                   \
  }

COLOR(title)
COLOR(sheet)
COLOR(group)
COLOR(mark)
#undef COLOR

#endif
