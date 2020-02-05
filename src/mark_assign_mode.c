#include <hikari/mark_assign_mode.h>

#include <wlr/types/wlr_seat.h>

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/mark.h>
#include <hikari/normal_mode.h>
#include <hikari/render_data.h>
#include <hikari/view.h>

static bool
check_confirmation(
    struct wlr_event_keyboard_key *event, struct hikari_keyboard *keyboard)
{
  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(
      keyboard->device->keyboard->xkb_state, keycode, &syms);

  for (int i = 0; i < nsyms; i++) {
    switch (syms[i]) {
      case XKB_KEY_Return:
        return true;
        break;
    }
  }

  return false;
}

static bool
check_cancellation(
    struct wlr_event_keyboard_key *event, struct hikari_keyboard *keyboard)
{
  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(
      keyboard->device->keyboard->xkb_state, keycode, &syms);

  for (int i = 0; i < nsyms; i++) {
    switch (syms[i]) {
      case XKB_KEY_Escape:
        return true;
        break;
    }
  }

  return false;
}

static struct hikari_mark *
lookup_mark(struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard,
    bool *selected)
{
  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(
      keyboard->device->keyboard->xkb_state, keycode, &syms);

  *selected = false;
  struct hikari_mark *mark = NULL;
  for (int i = 0; i < nsyms; i++) {
    switch (syms[i]) {
      case XKB_KEY_a:
        mark = HIKARI_MARK_a;
        *selected = true;
        break;

      case XKB_KEY_b:
        mark = HIKARI_MARK_b;
        *selected = true;
        break;

      case XKB_KEY_c:
        mark = HIKARI_MARK_c;
        *selected = true;
        break;

      case XKB_KEY_d:
        mark = HIKARI_MARK_d;
        *selected = true;
        break;

      case XKB_KEY_e:
        mark = HIKARI_MARK_e;
        *selected = true;
        break;

      case XKB_KEY_f:
        mark = HIKARI_MARK_f;
        *selected = true;
        break;

      case XKB_KEY_g:
        mark = HIKARI_MARK_g;
        *selected = true;
        break;

      case XKB_KEY_h:
        mark = HIKARI_MARK_h;
        *selected = true;
        break;

      case XKB_KEY_i:
        mark = HIKARI_MARK_i;
        *selected = true;
        break;

      case XKB_KEY_j:
        mark = HIKARI_MARK_j;
        *selected = true;
        break;

      case XKB_KEY_k:
        mark = HIKARI_MARK_k;
        *selected = true;
        break;

      case XKB_KEY_l:
        mark = HIKARI_MARK_l;
        *selected = true;
        break;

      case XKB_KEY_m:
        mark = HIKARI_MARK_m;
        *selected = true;
        break;

      case XKB_KEY_n:
        mark = HIKARI_MARK_n;
        *selected = true;
        break;

      case XKB_KEY_o:
        mark = HIKARI_MARK_o;
        *selected = true;
        break;

      case XKB_KEY_p:
        mark = HIKARI_MARK_p;
        *selected = true;
        break;

      case XKB_KEY_q:
        mark = HIKARI_MARK_q;
        *selected = true;
        break;

      case XKB_KEY_r:
        mark = HIKARI_MARK_r;
        *selected = true;
        break;

      case XKB_KEY_s:
        mark = HIKARI_MARK_s;
        *selected = true;
        break;

      case XKB_KEY_t:
        mark = HIKARI_MARK_t;
        *selected = true;
        break;

      case XKB_KEY_u:
        mark = HIKARI_MARK_u;
        *selected = true;
        break;

      case XKB_KEY_v:
        mark = HIKARI_MARK_v;
        *selected = true;
        break;

      case XKB_KEY_w:
        mark = HIKARI_MARK_w;
        *selected = true;
        break;

      case XKB_KEY_x:
        mark = HIKARI_MARK_x;
        *selected = true;
        break;

      case XKB_KEY_y:
        mark = HIKARI_MARK_y;
        *selected = true;
        break;

      case XKB_KEY_z:
        mark = HIKARI_MARK_z;
        *selected = true;
        break;

      case XKB_KEY_BackSpace:
        mark = NULL;
        *selected = true;
        break;
    }
  }

  return mark;
}

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
update_state(struct hikari_mark *mark,
    struct wlr_box *geometry,
    struct hikari_output *output)
{
  struct hikari_mark_assign_mode *mode = &hikari_server.mark_assign_mode;

  assert(mode == (struct hikari_mark_assign_mode *)hikari_server.mode);

  if (mark) {
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        mark->name,
        hikari_configuration.indicator_insert);
  } else {
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        " ",
        hikari_configuration.indicator_insert);
  }

  if (mode->pending_mark != mark) {
    clear_conflict();

    if (mark != NULL && mark->view != NULL) {
      hikari_indicator_update(&mode->indicator,
          mark->view,
          hikari_configuration.indicator_conflict);

      hikari_view_damage_whole(mark->view);
    }

    mode->pending_mark = mark;
  }
}

static void
confirm_mark_assign(struct wlr_box *geometry, struct hikari_output *output)
{
  struct hikari_mark_assign_mode *mode = &hikari_server.mark_assign_mode;

  assert(mode == (struct hikari_mark_assign_mode *)hikari_server.mode);

  struct hikari_view *focus_view = hikari_server.workspace->focus_view;
  struct hikari_mark *mark = mode->pending_mark;

  if (mark) {
    clear_conflict();

    hikari_mark_set(mark, focus_view);
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        mode->pending_mark->name,
        hikari_configuration.indicator_selected);

    mode->pending_mark = NULL;
  } else {
    if (focus_view->mark != NULL) {
      hikari_mark_clear(focus_view->mark);
    }

    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        "",
        hikari_configuration.indicator_selected);
  }

  hikari_server_enter_normal_mode(NULL);
}

static void
cancel_mark_assign(struct hikari_mark *mark,
    struct wlr_box *geometry,
    struct hikari_output *output)
{
  assert(&hikari_server.mark_assign_mode.mode == hikari_server.mode);

  struct hikari_view *view = hikari_server.workspace->focus_view;

  clear_conflict();

  if (view->mark != NULL) {
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        view->mark->name,
        hikari_configuration.indicator_selected);
  } else {
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        "",
        hikari_configuration.indicator_selected);
  }

  hikari_server_enter_normal_mode(NULL);
}

static void
assign_mark(struct hikari_workspace *workspace,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  assert(hikari_server.workspace->focus_view != NULL);

  bool selected;

  struct hikari_mark *mark = lookup_mark(event, keyboard, &selected);
  struct wlr_box *geometry = hikari_view_border_geometry(workspace->focus_view);
  struct hikari_output *output = workspace->output;

  if (selected) {
    update_state(mark, geometry, output);
    hikari_server_refresh_indication();
  } else if (check_confirmation(event, keyboard)) {
    confirm_mark_assign(geometry, output);
  } else if (check_cancellation(event, keyboard)) {
    cancel_mark_assign(mark, geometry, output);
    hikari_server_refresh_indication();
  }
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
        hikari_configuration.indicator_conflict,
        render_data);
    hikari_indicator_render(&mode->indicator, render_data);
  }

  if (view->output == output) {
    render_data->geometry = hikari_view_border_geometry(view);

    hikari_indicator_frame_render(&view->indicator_frame,
        hikari_configuration.indicator_selected,
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
  mark_assign_mode->mode.type = HIKARI_MODE_TYPE_ASSIGN_MARK;
  mark_assign_mode->mode.key_handler = key_handler;
  mark_assign_mode->mode.button_handler = button_handler;
  mark_assign_mode->mode.modifier_handler = modifier_handler;
  mark_assign_mode->mode.render = render;
  mark_assign_mode->mode.cancel = cancel;
  mark_assign_mode->mode.cursor_move = cursor_move;

  hikari_indicator_init(
      &mark_assign_mode->indicator, hikari_configuration.indicator_conflict);
}

void
hikari_mark_assign_mode_fini(struct hikari_mark_assign_mode *mark_assign_mode)
{
  hikari_indicator_fini(&mark_assign_mode->indicator);
}
