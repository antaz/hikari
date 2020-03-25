#include <hikari/mark_assign_mode.h>

#include <wlr/types/wlr_seat.h>

#include <hikari/configuration.h>
#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/mark.h>
#include <hikari/normal_mode.h>
#include <hikari/render_data.h>
#include <hikari/view.h>

static void
clear_conflict(void)
{
  struct hikari_mark_assign_mode *mode = &hikari_server.mark_assign_mode;

  assert(mode == (struct hikari_mark_assign_mode *)hikari_server.mode);

  struct hikari_mark *mark = mode->pending_mark;

  if (mark != NULL && mark->view != NULL) {
    hikari_view_damage_whole(mark->view);
    hikari_indicator_damage(&mode->indicator, mark->view);
  }
}

static void
update_state(struct hikari_mark *mark)
{
  struct hikari_mark_assign_mode *mode = &hikari_server.mark_assign_mode;

  assert(mode == (struct hikari_mark_assign_mode *)hikari_server.mode);

  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *view = workspace->focus_view;
  struct hikari_output *output = workspace->output;
  struct wlr_box *geometry = hikari_view_border_geometry(view);

  if (mark == NULL) {
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        " ",
        hikari_configuration->indicator_insert);
  } else {
    if (mode->pending_mark != mark) {
      clear_conflict();
      float *indicator_color;

      if (mark != NULL && mark->view != NULL) {
        hikari_indicator_update(&mode->indicator,
            mark->view,
            hikari_configuration->indicator_conflict);

        hikari_view_damage_whole(mark->view);

        indicator_color = hikari_configuration->indicator_conflict;
      } else {
        indicator_color = hikari_configuration->indicator_insert;
      }

      hikari_indicator_update_mark(&hikari_server.indicator,
          geometry,
          output,
          mark->name,
          indicator_color);
    }
  }

  mode->pending_mark = mark;
}

static void
confirm_mark_assign(void)
{
  struct hikari_mark_assign_mode *mode = &hikari_server.mark_assign_mode;

  assert(mode == (struct hikari_mark_assign_mode *)hikari_server.mode);

  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *view = workspace->focus_view;
  struct hikari_output *output = workspace->output;
  struct wlr_box *geometry = hikari_view_border_geometry(view);
  struct hikari_mark *mark = mode->pending_mark;

  if (mark != NULL) {
    clear_conflict();

    hikari_mark_set(mark, view);
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        mode->pending_mark->name,
        hikari_configuration->indicator_selected);

    mode->pending_mark = NULL;
  } else {
    if (view->mark != NULL) {
      hikari_mark_clear(view->mark);
    }

    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        "",
        hikari_configuration->indicator_selected);
  }

  hikari_server_enter_normal_mode(NULL);
}

static void
cancel_mark_assign(void)
{
  assert(&hikari_server.mark_assign_mode.mode == hikari_server.mode);

  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *view = workspace->focus_view;
  struct hikari_output *output = workspace->output;
  struct wlr_box *geometry = hikari_view_border_geometry(view);

  clear_conflict();

  if (view->mark != NULL) {
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        view->mark->name,
        hikari_configuration->indicator_selected);
  } else {
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        "",
        hikari_configuration->indicator_selected);
  }

  hikari_server_enter_normal_mode(NULL);
}

static void
handle_keysym(
    struct hikari_keyboard *keyboard, uint32_t keycode, xkb_keysym_t sym)
{
  uint32_t codepoint;
  struct hikari_mark *mark;

  switch (sym) {
    case XKB_KEY_Escape:
      cancel_mark_assign();
      hikari_server_refresh_indication();
      break;

    case XKB_KEY_Return:
      confirm_mark_assign();
      break;

    case XKB_KEY_BackSpace:
      update_state(NULL);
      hikari_server_refresh_indication();
      break;

    default:
      codepoint = hikari_keyboard_get_codepoint(keyboard, keycode);

      if (hikari_mark_get(codepoint, &mark)) {
        update_state(mark);
        hikari_server_refresh_indication();
      }
      break;
  }
}

static void
assign_mark(struct hikari_workspace *workspace,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  assert(hikari_server.workspace->focus_view != NULL);

  uint32_t keycode = event->keycode + 8;

  hikari_keyboard_for_keysym(keyboard, keycode, handle_keysym);
}

static void
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_event_keyboard_key *event = data;

  struct hikari_workspace *workspace = hikari_server.workspace;
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);

  if (event->state == WLR_KEY_PRESSED && modifiers == 0) {
    assign_mark(workspace, event, keyboard);
  }
}

static void
modifier_handler(struct wl_listener *listener, void *data)
{}

static void
render(struct hikari_output *output, struct hikari_render_data *render_data)
{
  assert(hikari_server.workspace->focus_view != NULL);
  struct hikari_view *view = hikari_server.workspace->focus_view;

  struct hikari_mark_assign_mode *mode =
      (struct hikari_mark_assign_mode *)hikari_server.mode;

  if (mode->pending_mark != NULL && mode->pending_mark->view != NULL &&
      mode->pending_mark->view->output == output) {
    render_data->geometry =
        hikari_view_border_geometry(mode->pending_mark->view);

    hikari_indicator_frame_render(&mode->pending_mark->view->indicator_frame,
        hikari_configuration->indicator_conflict,
        render_data);
    hikari_indicator_render(&mode->indicator, render_data);
  }

  if (view->output == output) {
    render_data->geometry = hikari_view_border_geometry(view);

    hikari_indicator_frame_render(&view->indicator_frame,
        hikari_configuration->indicator_selected,
        render_data);

    hikari_indicator_render(&hikari_server.indicator, render_data);
  }
}

static void
cancel(void)
{
  hikari_server.mark_assign_mode.pending_mark = NULL;
}

static void
button_handler(struct wl_listener *listener, void *data)
{}

static void
cursor_move(void)
{}

void
hikari_mark_assign_mode_init(struct hikari_mark_assign_mode *mark_assign_mode)
{
  mark_assign_mode->mode.key_handler = key_handler;
  mark_assign_mode->mode.button_handler = button_handler;
  mark_assign_mode->mode.modifier_handler = modifier_handler;
  mark_assign_mode->mode.render = render;
  mark_assign_mode->mode.cancel = cancel;
  mark_assign_mode->mode.cursor_move = cursor_move;

  hikari_indicator_init(
      &mark_assign_mode->indicator, hikari_configuration->indicator_conflict);
}

void
hikari_mark_assign_mode_fini(struct hikari_mark_assign_mode *mark_assign_mode)
{
  hikari_indicator_fini(&mark_assign_mode->indicator);
}
