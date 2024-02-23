#if !defined(HIKARI_MODE_H)
#define HIKARI_MODE_H

#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>

struct hikari_keyboard;
struct hikari_renderer;
struct hikari_cursor;

struct hikari_mode {
  void (*key_handler)(
      struct hikari_keyboard *keyboard, struct wlr_keyboard_key_event *event);

  void (*modifiers_handler)(struct hikari_keyboard *keyboard);

  void (*button_handler)(
      struct hikari_cursor *cursor, struct wlr_pointer_button_event *event);

  void (*cursor_move)(uint32_t time_msec);

  void (*cancel)(void);

  void (*render)(struct hikari_renderer *renderer);
};

#endif
