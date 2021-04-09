#if !defined(HIKARI_KEYBOARD_H)
#define HIKARI_KEYBOARD_H

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>

#include <hikari/binding_group.h>
#include <hikari/keyboard_config.h>

struct hikari_keyboard {
  struct wl_list server_keyboards;
  struct wlr_input_device *device;

  struct wl_listener modifiers;
  struct wl_listener key;
  struct wl_listener destroy;

  struct xkb_keymap *keymap;

  struct hikari_binding_group bindings[HIKARI_BINDING_GROUP_MASK];
};

void
hikari_keyboard_init(
    struct hikari_keyboard *keyboard, struct wlr_input_device *device);

void
hikari_keyboard_fini(struct hikari_keyboard *keyboard);

void
hikari_keyboard_configure(struct hikari_keyboard *keyboard,
    struct hikari_keyboard_config *keyboard_config);

void
hikari_keyboard_configure_bindings(
    struct hikari_keyboard *keyboard, struct wl_list *bindings);

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

static inline bool
hikari_keyboard_check_modifier(
    struct hikari_keyboard *keyboard, uint32_t modifier)
{
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);

  return modifiers == modifier;
}

#endif
