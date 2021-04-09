#include <hikari/resize_mode.h>

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
  struct hikari_view *view = hikari_server.workspace->focus_view;

  if (view != NULL) {
    struct hikari_indicator *indicator = &hikari_server.indicator;

    hikari_indicator_set_color(
        indicator, hikari_configuration->indicator_selected);
    hikari_indicator_update(indicator, view);
    hikari_indicator_damage(indicator, view);
    hikari_group_damage(view->group);

    hikari_view_center_cursor(view);
  }
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
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  assert(focus_view != NULL);

  struct hikari_output *output = focus_view->output;
  struct wlr_box *geometry = hikari_view_geometry(focus_view);

  int cursor_x = hikari_server.cursor.wlr_cursor->x - output->geometry.x;
  int cursor_y = hikari_server.cursor.wlr_cursor->y - output->geometry.y;

  int border = hikari_configuration->border;
  int new_width = cursor_x - geometry->x - border;
  int new_height = cursor_y - geometry->y - border;

  if (new_width > 0 && new_height > 0) {
    hikari_view_resize_absolute(focus_view, new_width, new_height);
  }
}

static void
button_handler(
    struct hikari_cursor *cursor, struct wlr_event_pointer_button *event)
{
  if (event->state == WLR_BUTTON_RELEASED) {
    hikari_server_enter_normal_mode(NULL);
  }
}

void
hikari_resize_mode_init(struct hikari_resize_mode *resize_mode)
{
  resize_mode->mode.key_handler = key_handler;
  resize_mode->mode.button_handler = button_handler;
  resize_mode->mode.modifiers_handler = modifiers_handler;
  resize_mode->mode.render = hikari_renderer_resize_mode;
  resize_mode->mode.cancel = cancel;
  resize_mode->mode.cursor_move = cursor_move;
}

void
hikari_resize_mode_enter(struct hikari_view *view)
{
  struct hikari_indicator *indicator = &hikari_server.indicator;

  hikari_indicator_set_color(indicator, hikari_configuration->indicator_insert);
  hikari_indicator_update(indicator, view);

  hikari_view_raise(view);
  hikari_view_bottom_right_cursor(view);

  hikari_server.mode = (struct hikari_mode *)&hikari_server.resize_mode;
}
