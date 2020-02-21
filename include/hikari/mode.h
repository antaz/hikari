#if !defined(HIKARI_MODE_H)
#define HIKARI_MODE_H

#include <wlr/types/wlr_keyboard.h>

struct hikari_keyboard;
struct hikari_output;
struct hikari_render_data;
struct hikari_workspace;

enum hikari_mode_type {
  HIKARI_MODE_TYPE_NORMAL,
  HIKARI_MODE_TYPE_GROUP_CHANGE,
  HIKARI_MODE_TYPE_ASSIGN_SHEET,
  HIKARI_MODE_TYPE_ASSIGN_MARK,
  HIKARI_MODE_TYPE_SELECT_MARK,
  HIKARI_MODE_TYPE_SELECT_EXEC,
  HIKARI_MODE_TYPE_GRAB_KEYBOARD,
  HIKARI_MODE_TYPE_MOVE,
  HIKARI_MODE_TYPE_TILING
};

struct hikari_mode {
  enum hikari_mode_type type;

  void (*key_handler)(struct wl_listener *listener, void *data);
  void (*modifier_handler)(struct wl_listener *listener, void *data);
  void (*button_handler)(struct wl_listener *listener, void *data);

  void (*cancel)(void);
  void (*cursor_move)(void);

  void (*render)(
      struct hikari_output *output, struct hikari_render_data *render_data);
};

#endif
