#include <hikari/dnd_mode.h>

#include <time.h>

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>

#include <hikari/binding.h>
#include <hikari/configuration.h>
#include <hikari/keyboard.h>
#include <hikari/renderer.h>
#include <hikari/server.h>
#include <hikari/view.h>

static void
cancel(void)
{
  wlr_seat_pointer_clear_focus(hikari_server.seat);
}

static void
key_handler(
    struct hikari_keyboard *keyboard, struct wlr_event_keyboard_key *event)
{
  if (event->state == WL_KEYBOARD_KEY_STATE_RELEASED) {
    hikari_server_enter_normal_mode(NULL);
  }
}

static void
modifiers_handler(struct hikari_keyboard *keyboard)
{}

static void
cursor_move(uint32_t time_msec)
{
  double x = hikari_server.cursor.wlr_cursor->x;
  double y = hikari_server.cursor.wlr_cursor->y;

  struct wlr_surface *surface;
  struct hikari_workspace *workspace;
  double sx, sy;
  struct hikari_node *node =
      hikari_server_node_at(x, y, &surface, &workspace, &sx, &sy);

  struct wlr_seat *seat = hikari_server.seat;

  if (node != NULL) {
    wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
    wlr_seat_pointer_notify_motion(seat, time_msec, sx, sy);
  } else {
    wlr_seat_pointer_clear_focus(seat);
  }
}

static void
button_handler(
    struct hikari_cursor *cursor, struct wlr_event_pointer_button *event)
{
  wlr_seat_pointer_notify_button(
      hikari_server.seat, event->time_msec, event->button, event->state);

  if (event->state == WLR_BUTTON_RELEASED) {
    hikari_server_enter_normal_mode(NULL);
  }
}

void
hikari_dnd_mode_init(struct hikari_dnd_mode *dnd_mode)
{
  dnd_mode->mode.key_handler = key_handler;
  dnd_mode->mode.button_handler = button_handler;
  dnd_mode->mode.modifiers_handler = modifiers_handler;
  dnd_mode->mode.render = hikari_renderer_dnd_mode;
  dnd_mode->mode.cancel = cancel;
  dnd_mode->mode.cursor_move = cursor_move;
}

void
hikari_dnd_mode_enter(void)
{
  hikari_server.mode = (struct hikari_mode *)&hikari_server.dnd_mode;
  cursor_move(0);
}
