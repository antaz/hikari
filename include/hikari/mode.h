#if !defined(HIKARI_MODE_H)
#define HIKARI_MODE_H

#include <wlr/types/wlr_keyboard.h>

struct hikari_keyboard;
struct hikari_output;
struct hikari_renderer;
struct hikari_workspace;

struct hikari_mode {
  void (*key_handler)(struct wl_listener *listener, void *data);
  void (*modifier_handler)(struct wl_listener *listener, void *data);
  void (*button_handler)(struct wl_listener *listener, void *data);

  void (*cancel)(void);

  void (*cursor_move)(uint32_t time_msec);

  void (*render)(struct hikari_renderer *renderer);
};

#endif
