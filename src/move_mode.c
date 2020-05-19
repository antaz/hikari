#include <hikari/move_mode.h>

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
  struct hikari_view *view = hikari_server.workspace->focus_view;

  if (view != NULL) {
    struct hikari_indicator *indicator = &hikari_server.indicator;
    hikari_indicator_update(
        indicator, view, hikari_configuration->indicator_selected);

    hikari_indicator_damage(indicator, view);
    hikari_group_damage(view->group);

    hikari_view_center_cursor(view);
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

  if (focus_view->output == output && !hikari_view_is_hidden(focus_view)) {
    render_data->geometry = hikari_view_border_geometry(focus_view);

    hikari_indicator_frame_render(&focus_view->indicator_frame,
        hikari_configuration->indicator_insert,
        render_data);

    hikari_indicator_render(&hikari_server.indicator, render_data);
  }
}

static void
cursor_move(uint32_t time_msec)
{
  double lx = hikari_server.cursor.wlr_cursor->x;
  double ly = hikari_server.cursor.wlr_cursor->y;

  struct wlr_output *wlr_output =
      wlr_output_layout_output_at(hikari_server.output_layout, lx, ly);

  if (wlr_output == NULL) {
    return;
  }

  struct hikari_output *output = wlr_output->data;
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  assert(focus_view != NULL);

  struct hikari_output *view_output = focus_view->output;

  if (output == view_output) {
    hikari_view_move_absolute(
        focus_view, lx - view_output->geometry.x, ly - view_output->geometry.y);
  } else {
    hikari_server_migrate_focus_view(
        output, lx, ly, hikari_configuration->indicator_insert);
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
hikari_move_mode_init(struct hikari_move_mode *move_mode)
{
  move_mode->mode.key_handler = key_handler;
  move_mode->mode.button_handler = button_handler;
  move_mode->mode.modifier_handler = modifier_handler;
  move_mode->mode.render = render;
  move_mode->mode.cancel = cancel;
  move_mode->mode.cursor_move = cursor_move;
}

void
hikari_move_mode_enter(struct hikari_view *view)
{
  hikari_indicator_update(
      &hikari_server.indicator, view, hikari_configuration->indicator_insert);

  hikari_view_raise(view);
  hikari_view_top_left_cursor(view);

  hikari_server.mode = (struct hikari_mode *)&hikari_server.move_mode;
}
