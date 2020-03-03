#include <hikari/tiling_mode.h>

#include <assert.h>

#include <wlr/types/wlr_seat.h>

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/layout.h>
#include <hikari/normal_mode.h>
#include <hikari/output.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/split.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

static void
tiling_key_handler(struct hikari_workspace *workspace,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);

  if ((modifiers & WLR_MODIFIER_LOGO) && event->state == WLR_KEY_PRESSED) {
    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(
        keyboard->device->keyboard->xkb_state, keycode, &syms);

    for (int i = 0; i < nsyms; i++) {
      switch (syms[i]) {
        case XKB_KEY_space:
          if (modifiers == WLR_MODIFIER_LOGO) {
            hikari_server_enter_normal_mode(NULL);
            hikari_output_damage_whole(workspace->output);
            hikari_server_cursor_focus();
            return;
          }
      }
    }
  }

  wlr_seat_set_keyboard(hikari_server.seat, keyboard->device);
  wlr_seat_keyboard_notify_key(
      hikari_server.seat, event->time_msec, event->keycode, event->state);
}

static void
render(struct hikari_output *output, struct hikari_render_data *render_data)
{
  struct hikari_workspace *workspace = hikari_server.workspace;

  assert(workspace->sheet->layout->split != NULL);
  hikari_split_render(workspace->sheet->layout->split, render_data);
}

static void
modifier_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard =
      wl_container_of(listener, keyboard, modifiers);

  wlr_seat_keyboard_notify_modifiers(
      hikari_server.seat, &keyboard->device->keyboard->modifiers);
}

static void
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);

  struct wlr_event_keyboard_key *event = data;
  struct hikari_workspace *workspace = hikari_server.workspace;

  tiling_key_handler(workspace, event, keyboard);
}

static void
cancel(void)
{}

static void
button_handler(struct wl_listener *listener, void *data)
{}

static void
cursor_move(void)
{}

void
hikari_tiling_mode_init(struct hikari_tiling_mode *tiling_mode)
{
  tiling_mode->mode.key_handler = key_handler;
  tiling_mode->mode.button_handler = button_handler;
  tiling_mode->mode.modifier_handler = modifier_handler;
  tiling_mode->mode.render = render;
  tiling_mode->mode.cancel = cancel;
  tiling_mode->mode.cursor_move = cursor_move;
}
