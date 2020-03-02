#include <hikari/keyboard.h>

#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>

#include <hikari/mark.h>
#include <hikari/memory.h>
#include <hikari/mode.h>
#include <hikari/server.h>
#include <hikari/unlocker.h>

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

  if (hikari_server.locked) {
    hikari_unlocker_key_handler(listener, data);
    return;
  }

  hikari_server.mode->key_handler(listener, data);
}

static void
modifiers_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard =
      wl_container_of(listener, keyboard, modifiers);

  update_mod_state(keyboard);

  if (hikari_server.locked) {
    return;
  }

  hikari_server.mode->modifier_handler(listener, data);
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

struct xkb_keymap *
hikari_load_keymap()
{

  struct xkb_rule_names rules = { 0 };
  rules.rules = getenv("XKB_DEFAULT_RULES");
  rules.model = getenv("XKB_DEFAULT_MODEL");
  rules.layout = getenv("XKB_DEFAULT_LAYOUT");
  rules.variant = getenv("XKB_DEFAULT_VARIANT");
  rules.options = getenv("XKB_DEFAULT_OPTIONS");

  struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_keymap *keymap =
      xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
  xkb_context_unref(context);
  return keymap;
}

void
hikari_keyboard_init(
    struct hikari_keyboard *keyboard, struct wlr_input_device *device)
{
  keyboard->device = device;

  struct xkb_keymap *keymap = hikari_load_keymap();
  wlr_keyboard_set_keymap(device->keyboard, keymap);
  xkb_keymap_unref(keymap);
  wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

  keyboard->modifiers.notify = modifiers_handler;
  wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);

  keyboard->key.notify = key_handler;
  wl_signal_add(&device->keyboard->events.key, &keyboard->key);

  keyboard->destroy.notify = destroy_handler;
  wl_signal_add(&device->keyboard->events.destroy, &keyboard->destroy);

  wlr_seat_set_keyboard(hikari_server.seat, device);

  wl_list_insert(&hikari_server.keyboards, &keyboard->link);
}

void
hikari_keyboard_fini(struct hikari_keyboard *keyboard)
{
  wl_list_remove(&keyboard->modifiers.link);
  wl_list_remove(&keyboard->key.link);
  wl_list_remove(&keyboard->destroy.link);
  wl_list_remove(&keyboard->link);
}

bool
hikari_keyboard_confirmation(
    struct hikari_keyboard *keyboard, struct wlr_event_keyboard_key *event)
{
  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(
      keyboard->device->keyboard->xkb_state, keycode, &syms);

  for (int i = 0; i < nsyms; i++) {
    switch (syms[i]) {
      case XKB_KEY_Return:
        return true;
        break;
    }
  }

  return false;
}

bool
hikari_keyboard_cancellation(
    struct hikari_keyboard *keyboard, struct wlr_event_keyboard_key *event)
{
  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(
      keyboard->device->keyboard->xkb_state, keycode, &syms);

  for (int i = 0; i < nsyms; i++) {
    switch (syms[i]) {
      case XKB_KEY_Escape:
        return true;
        break;
    }
  }

  return false;
}

struct hikari_mark *
hikari_keyboard_resolve_mark(struct hikari_keyboard *keyboard,
    struct wlr_event_keyboard_key *event,
    bool *selected)
{
  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(
      keyboard->device->keyboard->xkb_state, keycode, &syms);

  *selected = false;
  struct hikari_mark *mark = NULL;
  for (int i = 0; i < nsyms; i++) {
    switch (syms[i]) {
      case XKB_KEY_a:
        mark = HIKARI_MARK_a;
        *selected = true;
        break;

      case XKB_KEY_b:
        mark = HIKARI_MARK_b;
        *selected = true;
        break;

      case XKB_KEY_c:
        mark = HIKARI_MARK_c;
        *selected = true;
        break;

      case XKB_KEY_d:
        mark = HIKARI_MARK_d;
        *selected = true;
        break;

      case XKB_KEY_e:
        mark = HIKARI_MARK_e;
        *selected = true;
        break;

      case XKB_KEY_f:
        mark = HIKARI_MARK_f;
        *selected = true;
        break;

      case XKB_KEY_g:
        mark = HIKARI_MARK_g;
        *selected = true;
        break;

      case XKB_KEY_h:
        mark = HIKARI_MARK_h;
        *selected = true;
        break;

      case XKB_KEY_i:
        mark = HIKARI_MARK_i;
        *selected = true;
        break;

      case XKB_KEY_j:
        mark = HIKARI_MARK_j;
        *selected = true;
        break;

      case XKB_KEY_k:
        mark = HIKARI_MARK_k;
        *selected = true;
        break;

      case XKB_KEY_l:
        mark = HIKARI_MARK_l;
        *selected = true;
        break;

      case XKB_KEY_m:
        mark = HIKARI_MARK_m;
        *selected = true;
        break;

      case XKB_KEY_n:
        mark = HIKARI_MARK_n;
        *selected = true;
        break;

      case XKB_KEY_o:
        mark = HIKARI_MARK_o;
        *selected = true;
        break;

      case XKB_KEY_p:
        mark = HIKARI_MARK_p;
        *selected = true;
        break;

      case XKB_KEY_q:
        mark = HIKARI_MARK_q;
        *selected = true;
        break;

      case XKB_KEY_r:
        mark = HIKARI_MARK_r;
        *selected = true;
        break;

      case XKB_KEY_s:
        mark = HIKARI_MARK_s;
        *selected = true;
        break;

      case XKB_KEY_t:
        mark = HIKARI_MARK_t;
        *selected = true;
        break;

      case XKB_KEY_u:
        mark = HIKARI_MARK_u;
        *selected = true;
        break;

      case XKB_KEY_v:
        mark = HIKARI_MARK_v;
        *selected = true;
        break;

      case XKB_KEY_w:
        mark = HIKARI_MARK_w;
        *selected = true;
        break;

      case XKB_KEY_x:
        mark = HIKARI_MARK_x;
        *selected = true;
        break;

      case XKB_KEY_y:
        mark = HIKARI_MARK_y;
        *selected = true;
        break;

      case XKB_KEY_z:
        mark = HIKARI_MARK_z;
        *selected = true;
        break;

      case XKB_KEY_BackSpace:
        mark = NULL;
        *selected = true;
        break;
    }
  }

  return mark;
}
