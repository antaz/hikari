#include <hikari/dnd_mode.h>

#include <time.h>

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>

#include <hikari/configuration.h>
#include <hikari/keybinding.h>
#include <hikari/keyboard.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/view.h>

static void
cancel(void)
{}

static void
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);

  struct wlr_event_keyboard_key *event = data;

  if (event->state == WLR_KEY_RELEASED) {
    hikari_server_enter_normal_mode(NULL);
  }
}

static void
modifier_handler(struct wl_listener *listener, void *data)
{}

static void
render(struct hikari_output *output, struct hikari_render_data *render_data)
{}

static void
cursor_move(uint32_t time_msec)
{
  double x = hikari_server.cursor->x;
  double y = hikari_server.cursor->y;

  struct wlr_surface *surface = NULL;
  double sx, sy;
  struct hikari_view_interface *view_interface =
      hikari_server_view_interface_at(x, y, &surface, &sx, &sy);

  if (view_interface != NULL) {
    wlr_seat_pointer_notify_enter(hikari_server.seat, surface, sx, sy);
  }

  wlr_seat_pointer_notify_motion(hikari_server.seat, time_msec, x, y);
}

static void
button_handler(struct wl_listener *listener, void *data)
{
  struct wlr_event_pointer_button *event = data;

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
  dnd_mode->mode.modifier_handler = modifier_handler;
  dnd_mode->mode.render = render;
  dnd_mode->mode.cancel = cancel;
  dnd_mode->mode.cursor_move = cursor_move;
}

void
hikari_dnd_mode_enter(void)
{
  hikari_server.mode = (struct hikari_mode *)&hikari_server.dnd_mode;
}
