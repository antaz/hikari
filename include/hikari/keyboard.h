#if !defined(HIKARI_KEYBOARD_H)
#define HIKARI_KEYBOARD_H

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <wlr/types/wlr_input_device.h>

struct hikari_mark;

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

typedef void (*hikari_keysym_iterator)(
    struct hikari_keyboard *keyboard, uint32_t keycode, xkb_keysym_t sym);

void
hikari_keyboard_for_keysym(struct hikari_keyboard *keyboard,
    uint32_t keycode,
    hikari_keysym_iterator iter);

static inline uint32_t
hikari_keyboard_get_codepoint(
    struct hikari_keyboard *keyboard, uint32_t keycode)
{
  return xkb_state_key_get_utf32(
      keyboard->device->keyboard->xkb_state, keycode);
}

#endif
