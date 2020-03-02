#if !defined(HIKARI_KEYBOARD_H)
#define HIKARI_KEYBOARD_H

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <wlr/types/wlr_input_device.h>

struct hikari_keyboard {
  struct wl_list link;
  struct wlr_input_device *device;

  struct wl_listener modifiers;
  struct wl_listener key;
  struct wl_listener destroy;
};

void
hikari_keyboard_init(
    struct hikari_keyboard *keyboard, struct wlr_input_device *device);

void
hikari_keyboard_fini(struct hikari_keyboard *keyboard);

struct xkb_keymap *
hikari_load_keymap();

bool
hikari_keyboard_confirmation(
    struct hikari_keyboard *keyboard, struct wlr_event_keyboard_key *event);

bool
hikari_keyboard_cancellation(
    struct hikari_keyboard *keyboard, struct wlr_event_keyboard_key *event);

#endif
