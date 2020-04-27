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

static struct hikari_mark_assign_mode *
get_mode(void)
{
  struct hikari_mark_assign_mode *mode = &hikari_server.mark_assign_mode;

  assert(mode == (struct hikari_mark_assign_mode *)hikari_server.mode);

  return mode;
}

static void
clear_conflict(void)
{
  struct hikari_mark_assign_mode *mode = get_mode();
  struct hikari_mark *mark = mode->pending_mark;

  if (mark != NULL && mark->view != NULL) {
    hikari_view_damage_border(mark->view);
    hikari_indicator_damage(&mode->indicator, mark->view);
  }
}

static void
update_state(struct hikari_mark *mark)
{
  struct hikari_mark_assign_mode *mode = get_mode();
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *view = workspace->focus_view;
  struct hikari_output *output = workspace->output;
  struct wlr_box *geometry = hikari_view_border_geometry(view);
  struct hikari_indicator *indicator = &hikari_server.indicator;

  if (mark == NULL) {
    hikari_indicator_update_mark(
        indicator, output, " ", hikari_configuration->indicator_insert);
  } else {
    if (mode->pending_mark != mark) {
      clear_conflict();
      float *indicator_color;
      struct hikari_view *mark_view = mark->view;

      if (mark != NULL && mark_view != NULL) {
        hikari_indicator_update(&mode->indicator,
            mark_view,
            hikari_configuration->indicator_conflict);

        hikari_view_damage_border(mark_view);
        hikari_indicator_damage(&mode->indicator, mark_view);

        indicator_color = hikari_configuration->indicator_conflict;
      } else {
        indicator_color = hikari_configuration->indicator_insert;
      }

      hikari_indicator_update_mark(
          indicator, output, mark->name, indicator_color);

      hikari_indicator_damage_mark(indicator, output, geometry);
    }
  }

  mode->pending_mark = mark;
}

static void
confirm_mark_assign(void)
{
  struct hikari_mark_assign_mode *mode = get_mode();
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *view = workspace->focus_view;
  struct hikari_output *output = workspace->output;
  struct hikari_mark *mark = mode->pending_mark;
  struct hikari_indicator *indicator = &hikari_server.indicator;

  if (mark != NULL) {
    clear_conflict();

    hikari_mark_set(mark, view);
    hikari_indicator_update_mark(indicator,
        output,
        mode->pending_mark->name,
        hikari_configuration->indicator_selected);

    mode->pending_mark = NULL;
  } else {
    if (view->mark != NULL) {
      hikari_mark_clear(view->mark);
    }

    hikari_indicator_update_mark(
        indicator, output, "", hikari_configuration->indicator_selected);
  }

  hikari_server_enter_normal_mode(NULL);
}

static void
select_mark(struct hikari_keyboard *keyboard, uint32_t keycode)
{
  uint32_t codepoint = hikari_keyboard_get_codepoint(keyboard, keycode);

  struct hikari_mark *mark;
  if (hikari_mark_get(codepoint, &mark)) {
    update_state(mark);
  }
}

static void
handle_keysym(
    struct hikari_keyboard *keyboard, uint32_t keycode, xkb_keysym_t sym)
{
  switch (sym) {
    case XKB_KEY_c:
    case XKB_KEY_d:
      if (!hikari_keyboard_check_modifier(keyboard, WLR_MODIFIER_CTRL)) {
        select_mark(keyboard, keycode);
        break;
      }
    case XKB_KEY_Escape:
      hikari_server_enter_normal_mode(NULL);
      break;

    case XKB_KEY_Return:
      confirm_mark_assign();
      break;

    case XKB_KEY_BackSpace:
      update_state(NULL);
      break;

    default:
      select_mark(keyboard, keycode);
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

  if (event->state == WLR_KEY_PRESSED) {
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
  struct hikari_mark_assign_mode *mode = get_mode();

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
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *view = workspace->focus_view;

  clear_conflict();

  if (view != NULL) {
    struct hikari_indicator *indicator = &hikari_server.indicator;
    struct hikari_output *output = view->output;

    if (view->mark != NULL) {
      hikari_indicator_update_mark(indicator,
          output,
          view->mark->name,
          hikari_configuration->indicator_selected);
    } else {
      hikari_indicator_update_mark(
          indicator, output, "", hikari_configuration->indicator_selected);
    }

    hikari_indicator_damage(indicator, view);
    hikari_view_damage_border(view);
  }

  hikari_server.mark_assign_mode.pending_mark = NULL;
}

static void
button_handler(struct wl_listener *listener, void *data)
{}

static void
cursor_move(uint32_t time_msec)
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

void
hikari_mark_assign_mode_enter(struct hikari_view *view)
{
  assert(view == hikari_server.workspace->focus_view);

  struct wlr_box *geometry = hikari_view_border_geometry(view);
  struct hikari_output *output = view->output;
  struct hikari_mark *mark = view->mark;
  struct hikari_indicator *indicator = &hikari_server.indicator;

  hikari_server.mark_assign_mode.pending_mark = NULL;
  hikari_server.mode = (struct hikari_mode *)&hikari_server.mark_assign_mode;

  if (mark == NULL) {
    hikari_indicator_update_mark(
        indicator, output, " ", hikari_configuration->indicator_insert);
  } else {
    hikari_indicator_update_mark(
        indicator, output, mark->name, hikari_configuration->indicator_insert);
  }

  hikari_indicator_damage_mark(indicator, output, geometry);
}
