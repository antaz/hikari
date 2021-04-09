#include <hikari/input_grab_mode.h>

#include <assert.h>

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_seat.h>

#include <hikari/action.h>
#include <hikari/binding.h>
#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/normal_mode.h>
#include <hikari/output.h>
#include <hikari/renderer.h>
#include <hikari/server.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

static void
modifiers_handler(struct hikari_keyboard *keyboard)
{
  wlr_seat_keyboard_notify_modifiers(
      hikari_server.seat, &keyboard->device->keyboard->modifiers);
}

static bool
handle_input(struct hikari_binding_group *map, uint32_t code)
{
  int nbindings = map->nbindings;
  struct hikari_binding *bindings = map->bindings;

  for (int i = 0; i < nbindings; i++) {
    struct hikari_binding *binding = &bindings[i];

    if (binding->keycode == code) {
      struct hikari_event_action *event_action = &binding->action->begin;

      return event_action->action == hikari_server_enter_input_grab_mode;
    }
  }

  return false;
}

static void
key_handler(
    struct hikari_keyboard *keyboard, struct wlr_event_keyboard_key *event)
{
  struct hikari_workspace *workspace = hikari_server.workspace;
  if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    uint32_t modifiers = hikari_server.keyboard_state.modifiers;
    struct hikari_binding_group *bindings = &keyboard->bindings[modifiers];

    if (handle_input(bindings, event->keycode)) {
      hikari_server_enter_normal_mode(NULL);
      hikari_view_damage_border(workspace->focus_view);
      hikari_server_cursor_focus();
      return;
    }
  }

  wlr_seat_set_keyboard(hikari_server.seat, keyboard->device);
  wlr_seat_keyboard_notify_key(
      hikari_server.seat, event->time_msec, event->keycode, event->state);
}

static void
cancel(void)
{}

static void
button_handler(
    struct hikari_cursor *cursor, struct wlr_event_pointer_button *event)
{
  wlr_seat_pointer_notify_button(
      hikari_server.seat, event->time_msec, event->button, event->state);
}

static void
cursor_move(uint32_t time_msec)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;
  assert(focus_view != NULL);
  struct hikari_node *node = (struct hikari_node *)focus_view;
  struct hikari_output *output = focus_view->output;
  assert(output != NULL);

  double lx = hikari_server.cursor.wlr_cursor->x;
  double ly = hikari_server.cursor.wlr_cursor->y;
  double ox = lx - output->geometry.x;
  double oy = ly - output->geometry.y;

  double sx, sy;

  struct wlr_surface *surface = hikari_node_surface_at(node, ox, oy, &sx, &sy);

  if (surface != NULL) {
    wlr_seat_pointer_notify_enter(hikari_server.seat, surface, sx, sy);
    wlr_seat_pointer_notify_motion(hikari_server.seat, time_msec, sx, sy);
  }
}

void
hikari_input_grab_mode_init(struct hikari_input_grab_mode *input_grab_mode)
{
  input_grab_mode->mode.key_handler = key_handler;
  input_grab_mode->mode.button_handler = button_handler;
  input_grab_mode->mode.modifiers_handler = modifiers_handler;
  input_grab_mode->mode.render = hikari_renderer_input_grab_mode;
  input_grab_mode->mode.cancel = cancel;
  input_grab_mode->mode.cursor_move = cursor_move;
}

void
hikari_input_grab_mode_enter(struct hikari_view *view)
{
  hikari_server.mode = (struct hikari_mode *)&hikari_server.input_grab_mode;
}
