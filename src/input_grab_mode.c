#include <hikari/input_grab_mode.h>

#include <assert.h>

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_seat.h>

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/normal_mode.h>
#include <hikari/output.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

static void
input_grab_key_handler(struct hikari_workspace *workspace,
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
        case XKB_KEY_g:
          if (modifiers ==
              (WLR_MODIFIER_LOGO | WLR_MODIFIER_ALT | WLR_MODIFIER_CTRL)) {
            hikari_server_enter_normal_mode(NULL);
            hikari_view_damage_border(workspace->focus_view);
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
  assert(hikari_server.workspace->focus_view != NULL);

  struct hikari_view *view = hikari_server.workspace->focus_view;

  if (view->output == output) {
    render_data->geometry = hikari_view_border_geometry(view);
    hikari_indicator_frame_render(&view->indicator_frame,
        hikari_configuration->indicator_insert,
        render_data);
  }
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

  input_grab_key_handler(workspace, event, keyboard);
}

static void
cancel(void)
{}

static void
button_handler(struct wl_listener *listener, void *data)
{
  struct wlr_event_pointer_button *event = data;

  wlr_seat_pointer_notify_button(
      hikari_server.seat, event->time_msec, event->button, event->state);
}

static void
cursor_move(uint32_t time_msec)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;
  assert(focus_view != NULL);
  struct hikari_view_interface *view_interface =
      (struct hikari_view_interface *)focus_view;
  struct hikari_output *output = focus_view->output;
  assert(output != NULL);

  double lx = hikari_server.cursor->x;
  double ly = hikari_server.cursor->y;
  double ox = lx - output->geometry.x;
  double oy = ly - output->geometry.y;

  double sx, sy;

  struct wlr_surface *surface =
      hikari_view_interface_surface_at(view_interface, ox, oy, &sx, &sy);

  wlr_seat_pointer_notify_enter(hikari_server.seat, surface, sx, sy);
  wlr_seat_pointer_notify_motion(hikari_server.seat, time_msec, sx, sy);
}

void
hikari_input_grab_mode_init(struct hikari_input_grab_mode *input_grab_mode)
{
  input_grab_mode->mode.key_handler = key_handler;
  input_grab_mode->mode.button_handler = button_handler;
  input_grab_mode->mode.modifier_handler = modifier_handler;
  input_grab_mode->mode.render = render;
  input_grab_mode->mode.cancel = cancel;
  input_grab_mode->mode.cursor_move = cursor_move;
}

void
hikari_input_grab_mode_enter(struct hikari_view *view)
{
  hikari_server.mode = (struct hikari_mode *)&hikari_server.input_grab_mode;
}
