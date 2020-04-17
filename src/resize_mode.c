#include <hikari/resize_mode.h>

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
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view != NULL) {
    hikari_indicator_update(&hikari_server.indicator,
        focus_view,
        hikari_configuration->indicator_selected);

    hikari_view_center_cursor(focus_view);
  }
}

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
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view->output == output) {
    render_data->geometry = hikari_view_border_geometry(focus_view);

    hikari_indicator_frame_render(&focus_view->indicator_frame,
        hikari_configuration->indicator_insert,
        render_data);

    hikari_indicator_render(&hikari_server.indicator, render_data);
  }
}

static void
cursor_move(void)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  assert(focus_view != NULL);

  struct hikari_output *output = focus_view->output;
  struct wlr_box *geometry = hikari_view_geometry(focus_view);

  int cursor_x = hikari_server.cursor->x - output->geometry.x;
  int cursor_y = hikari_server.cursor->y - output->geometry.y;

  int border = hikari_configuration->border;
  int new_width = cursor_x - geometry->x - border;
  int new_height = cursor_y - geometry->y - border;

  if (new_width > 0 && new_height > 0) {
    hikari_view_resize_absolute(focus_view, new_width, new_height);
  }
}

static void
button_handler(struct wl_listener *listener, void *data)
{
  struct wlr_event_pointer_button *event = data;

  if (event->state == WLR_BUTTON_RELEASED) {
    hikari_server_enter_normal_mode(NULL);
  }
}

void
hikari_resize_mode_init(struct hikari_resize_mode *resize_mode)
{
  resize_mode->mode.key_handler = key_handler;
  resize_mode->mode.button_handler = button_handler;
  resize_mode->mode.modifier_handler = modifier_handler;
  resize_mode->mode.render = render;
  resize_mode->mode.cancel = cancel;
  resize_mode->mode.cursor_move = cursor_move;
}

void
hikari_resize_mode_enter(struct hikari_view *view)
{
  hikari_indicator_update(
      &hikari_server.indicator, view, hikari_configuration->indicator_insert);

  hikari_view_raise(view);
  hikari_view_bottom_right_cursor(view);

  hikari_server.mode = (struct hikari_mode *)&hikari_server.resize_mode;
  hikari_server_refresh_indication();
}
