#include <hikari/mark_select_mode.h>

#include <stdbool.h>

#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>

#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/mark.h>
#include <hikari/normal_mode.h>
#include <hikari/render_data.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

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
mark_select(struct hikari_workspace *workspace,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  bool selected;
  struct hikari_mark *mark = lookup_mark(event, keyboard, &selected);

  hikari_server_enter_normal_mode(NULL);

  if (mark != NULL && mark->view != NULL) {
    struct hikari_view *view = mark->view;

    if (hikari_server.mark_select_mode.switch_workspace) {
      hikari_workspace_switch_sheet(view->sheet->workspace, view->sheet);
    }

    if (!hikari_view_is_hidden(view)) {
      hikari_view_raise(view);
    } else {
      hikari_view_raise_hidden(view);
      hikari_view_show(view);
    }

    if (view != hikari_server.workspace->focus_view) {
      hikari_workspace_focus_view(workspace, view);
    }

    hikari_view_center_cursor(view);
  }

  hikari_server_cursor_focus();
}

static void
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_event_keyboard_key *event = data;

  struct hikari_workspace *workspace = hikari_server.workspace;

  if (event->state == WLR_KEY_PRESSED) {
    mark_select(workspace, event, keyboard);
  }
}

static void
modifier_handler(struct wl_listener *listener, void *data)
{}

static void
render(struct hikari_output *output, struct hikari_render_data *render_data)
{}

static void
cancel(void)
{}

static void
button_handler(struct wl_listener *listener, void *data)
{}

static void
cursor_move(void)
{}

void
hikari_mark_select_mode_init(struct hikari_mark_select_mode *mark_select_mode)
{
  mark_select_mode->mode.type = HIKARI_MODE_TYPE_SELECT_MARK;
  mark_select_mode->mode.key_handler = key_handler;
  mark_select_mode->mode.button_handler = button_handler;
  mark_select_mode->mode.modifier_handler = modifier_handler;
  mark_select_mode->mode.render = render;
  mark_select_mode->mode.cancel = cancel;
  mark_select_mode->mode.cursor_move = cursor_move;

  mark_select_mode->switch_workspace = false;
}
