#if !defined(HIKARI_CURSOR_H)
#define HIKARI_CURSOR_H

#include <wlr/types/wlr_cursor.h>

struct hikari_output;

struct hikari_cursor {
  struct wlr_cursor *wlr_cursor;

  struct wl_listener motion_absolute;
  struct wl_listener motion;
  struct wl_listener frame;
  struct wl_listener axis;
  struct wl_listener button;
  struct wl_listener surface_destroy;
  struct wl_listener request_set_cursor;
};

void
hikari_cursor_init(struct hikari_cursor *cursor, struct wlr_cursor *wlr_cursor);

void
hikari_cursor_fini(struct hikari_cursor *cursor);

void
hikari_cursor_activate(struct hikari_cursor *cursor);

void
hikari_cursor_deactivate(struct hikari_cursor *cursor);

void
hikari_cursor_set_image(struct hikari_cursor *cursor, const char *path);

void
hikari_cursor_center(struct hikari_cursor *cursor,
    struct hikari_output *output,
    struct wlr_box *geometry);

static inline void
hikari_cursor_reset_image(struct hikari_cursor *cursor)
{
  hikari_cursor_set_image(cursor, "left_ptr");
}

static inline void
hikari_cursor_warp(struct hikari_cursor *cursor, int x, int y)
{
  wlr_cursor_warp(cursor->wlr_cursor, NULL, x, y);
}

#endif
