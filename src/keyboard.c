#include <hikari/keyboard.h>

#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>

#include <hikari/binding.h>
#include <hikari/binding_config.h>
#include <hikari/keyboard_config.h>
#include <hikari/memory.h>
#include <hikari/mode.h>
#include <hikari/server.h>

static void
update_mod_state(struct hikari_keyboard *keyboard)
{
  uint32_t modifier_keys =
      wlr_keyboard_get_modifiers(keyboard->device->keyboard);

  bool was_pressed = hikari_server.keyboard_state.mod_pressed;
  bool is_pressed = modifier_keys & WLR_MODIFIER_LOGO;

  hikari_server.keyboard_state.modifiers = modifier_keys;
  hikari_server.keyboard_state.mod_released = was_pressed && !is_pressed;
  hikari_server.keyboard_state.mod_changed = was_pressed != is_pressed;
  hikari_server.keyboard_state.mod_pressed = is_pressed;
}

static void
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_event_keyboard_key *event = data;

  hikari_server.mode->key_handler(keyboard, event);
}

static void
modifiers_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard =
      wl_container_of(listener, keyboard, modifiers);

  update_mod_state(keyboard);

  hikari_server.mode->modifiers_handler(keyboard);
}

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard =
      wl_container_of(listener, keyboard, destroy);

  hikari_keyboard_fini(keyboard);
  hikari_free(keyboard);

  /* uint32_t caps = WL_SEAT_CAPABILITY_POINTER; */
  /* wlr_seat_set_capabilities(hikari_server.seat, caps); */
}

struct keycode_matcher_state {
  xkb_keysym_t keysym;
  uint32_t *keycode;
  struct xkb_state *state;
};

static void
match_keycode(struct xkb_keymap *keymap, xkb_keycode_t key, void *data)
{
  struct keycode_matcher_state *matcher_state = data;
  xkb_keysym_t keysym = xkb_state_key_get_one_sym(matcher_state->state, key);

  if (keysym != XKB_KEY_NoSymbol && keysym == matcher_state->keysym &&
      *(matcher_state->keycode) == 0) {
    *(matcher_state->keycode) = key - 8;
  }
}

static void
resolve_keysym(uint32_t *keycode, struct xkb_state *state, xkb_keysym_t keysym)
{
  struct keycode_matcher_state matcher_state = { keysym, keycode, state };

  xkb_keymap_key_for_each(
      xkb_state_get_keymap(state), match_keycode, &matcher_state);
}

static void
configure_bindings(struct hikari_keyboard *keyboard, struct wl_list *bindings)
{
  int nr[256] = { 0 };
  struct hikari_binding_config *binding_config;
  wl_list_for_each (binding_config, bindings, link) {
    nr[binding_config->key.modifiers]++;
  }

  for (int mask = 0; mask < 256; mask++) {
    keyboard->bindings[mask].nbindings = nr[mask];
    if (nr[mask] != 0) {
      keyboard->bindings[mask].bindings =
          hikari_calloc(nr[mask], sizeof(struct hikari_binding));
    } else {
      keyboard->bindings[mask].bindings = NULL;
    }

    nr[mask] = 0;
  }

  struct xkb_state *state = xkb_state_new(keyboard->keymap);

  wl_list_for_each (binding_config, bindings, link) {
    uint8_t mask = binding_config->key.modifiers;
    struct hikari_binding *binding =
        &keyboard->bindings[mask].bindings[nr[mask]];

    binding->action = &binding_config->action;

    switch (binding_config->key.type) {
      case HIKARI_ACTION_BINDING_KEY_KEYCODE:
        binding->keycode = binding_config->key.value.keycode;
        break;

      case HIKARI_ACTION_BINDING_KEY_KEYSYM:
        resolve_keysym(
            &binding->keycode, state, binding_config->key.value.keysym);
        break;
    }

    nr[mask]++;
  }

  xkb_state_unref(state);
}

void
hikari_keyboard_init(
    struct hikari_keyboard *keyboard, struct wlr_input_device *device)
{
  keyboard->device = device;
  keyboard->keymap = NULL;

  keyboard->modifiers.notify = modifiers_handler;
  wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);

  keyboard->key.notify = key_handler;
  wl_signal_add(&device->keyboard->events.key, &keyboard->key);

  keyboard->destroy.notify = destroy_handler;
  wl_signal_add(&device->keyboard->events.destroy, &keyboard->destroy);

  wlr_seat_set_keyboard(hikari_server.seat, device);

  wl_list_insert(&hikari_server.keyboards, &keyboard->server_keyboards);

  hikari_binding_group_init(keyboard->bindings);
}

void
hikari_keyboard_fini(struct hikari_keyboard *keyboard)
{
  wl_list_remove(&keyboard->modifiers.link);
  wl_list_remove(&keyboard->key.link);
  wl_list_remove(&keyboard->destroy.link);

  wl_list_remove(&keyboard->server_keyboards);

  xkb_keymap_unref(keyboard->keymap);
  hikari_binding_group_fini(keyboard->bindings);
}

static struct xkb_keymap *
load_keymap(struct hikari_keyboard_config *keyboard_config)
{
  struct xkb_keymap *keymap = NULL;

  switch (keyboard_config->xkb.type) {
    case HIKARI_XKB_TYPE_RULES:
      assert(false);
    case HIKARI_XKB_TYPE_KEYMAP:
      keymap = xkb_keymap_ref(keyboard_config->xkb.value.keymap);
      break;
  }

  return keymap;
}

void
hikari_keyboard_configure(struct hikari_keyboard *keyboard,
    struct hikari_keyboard_config *keyboard_config)
{
  keyboard->keymap = load_keymap(keyboard_config);
  assert(keyboard->keymap != NULL);
  wlr_keyboard_set_keymap(keyboard->device->keyboard, keyboard->keymap);

  int repeat_rate = hikari_keyboard_config_get_repeat_rate(keyboard_config);
  int repeat_delay = hikari_keyboard_config_get_repeat_delay(keyboard_config);

  wlr_keyboard_set_repeat_info(
      keyboard->device->keyboard, repeat_rate, repeat_delay);
}

void
hikari_keyboard_configure_bindings(
    struct hikari_keyboard *keyboard, struct wl_list *bindings)
{
  hikari_binding_group_fini(keyboard->bindings);
  hikari_binding_group_init(keyboard->bindings);

  configure_bindings(keyboard, bindings);
}

void
hikari_keyboard_for_keysym(struct hikari_keyboard *keyboard,
    uint32_t keycode,
    hikari_keysym_iterator iter)
{
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(
      keyboard->device->keyboard->xkb_state, keycode, &syms);

  for (int i = 0; i < nsyms; i++) {
    iter(keyboard, keycode, syms[i]);
  }
}
