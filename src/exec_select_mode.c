#include <hikari/exec_select_mode.h>

#include <stdbool.h>

#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>

#include <hikari/command.h>
#include <hikari/exec.h>
#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/normal_mode.h>
#include <hikari/render_data.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

static struct hikari_exec *
lookup_exec(struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard,
    bool *selected)
{
  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(
      keyboard->device->keyboard->xkb_state, keycode, &syms);

  *selected = false;
  struct hikari_exec *exec = NULL;
  for (int i = 0; i < nsyms; i++) {
    switch (syms[i]) {
      case XKB_KEY_a:
        exec = HIKARI_EXEC_a;
        *selected = true;
        break;

      case XKB_KEY_b:
        exec = HIKARI_EXEC_b;
        *selected = true;
        break;

      case XKB_KEY_c:
        exec = HIKARI_EXEC_c;
        *selected = true;
        break;

      case XKB_KEY_d:
        exec = HIKARI_EXEC_d;
        *selected = true;
        break;

      case XKB_KEY_e:
        exec = HIKARI_EXEC_e;
        *selected = true;
        break;

      case XKB_KEY_f:
        exec = HIKARI_EXEC_f;
        *selected = true;
        break;

      case XKB_KEY_g:
        exec = HIKARI_EXEC_g;
        *selected = true;
        break;

      case XKB_KEY_h:
        exec = HIKARI_EXEC_h;
        *selected = true;
        break;

      case XKB_KEY_i:
        exec = HIKARI_EXEC_i;
        *selected = true;
        break;

      case XKB_KEY_j:
        exec = HIKARI_EXEC_j;
        *selected = true;
        break;

      case XKB_KEY_k:
        exec = HIKARI_EXEC_k;
        *selected = true;
        break;

      case XKB_KEY_l:
        exec = HIKARI_EXEC_l;
        *selected = true;
        break;

      case XKB_KEY_m:
        exec = HIKARI_EXEC_m;
        *selected = true;
        break;

      case XKB_KEY_n:
        exec = HIKARI_EXEC_n;
        *selected = true;
        break;

      case XKB_KEY_o:
        exec = HIKARI_EXEC_o;
        *selected = true;
        break;

      case XKB_KEY_p:
        exec = HIKARI_EXEC_p;
        *selected = true;
        break;

      case XKB_KEY_q:
        exec = HIKARI_EXEC_q;
        *selected = true;
        break;

      case XKB_KEY_r:
        exec = HIKARI_EXEC_r;
        *selected = true;
        break;

      case XKB_KEY_s:
        exec = HIKARI_EXEC_s;
        *selected = true;
        break;

      case XKB_KEY_t:
        exec = HIKARI_EXEC_t;
        *selected = true;
        break;

      case XKB_KEY_u:
        exec = HIKARI_EXEC_u;
        *selected = true;
        break;

      case XKB_KEY_v:
        exec = HIKARI_EXEC_v;
        *selected = true;
        break;

      case XKB_KEY_w:
        exec = HIKARI_EXEC_w;
        *selected = true;
        break;

      case XKB_KEY_x:
        exec = HIKARI_EXEC_x;
        *selected = true;
        break;

      case XKB_KEY_y:
        exec = HIKARI_EXEC_y;
        *selected = true;
        break;

      case XKB_KEY_z:
        exec = HIKARI_EXEC_z;
        *selected = true;
        break;

      case XKB_KEY_BackSpace:
        exec = NULL;
        *selected = true;
        break;
    }
  }

  return exec;
}

static void
exec_select(struct hikari_workspace *workspace,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  bool selected;
  struct hikari_exec *exec = lookup_exec(event, keyboard, &selected);

  hikari_server_enter_normal_mode(NULL);

  if (exec != NULL && exec->command != NULL) {
    hikari_command_execute(exec->command);
  }
}

static void
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_event_keyboard_key *event = data;

  struct hikari_workspace *workspace = hikari_server.workspace;

  if (event->state == WLR_KEY_PRESSED) {
    exec_select(workspace, event, keyboard);
  }
}

static void
modifier_handler(struct wl_listener *listener, void *data)
{}

static void
button_handler(struct wl_listener *listener, void *data)
{}

static void
render(struct hikari_output *output, struct hikari_render_data *render_data)
{}

static void
cancel(void)
{}

static void
cursor_move(void)
{}

void
hikari_exec_select_mode_init(struct hikari_exec_select_mode *exec_select_mode)
{
  exec_select_mode->mode.type = HIKARI_MODE_TYPE_SELECT_EXEC;
  exec_select_mode->mode.key_handler = key_handler;
  exec_select_mode->mode.button_handler = button_handler;
  exec_select_mode->mode.modifier_handler = modifier_handler;
  exec_select_mode->mode.render = render;
  exec_select_mode->mode.cancel = cancel;
  exec_select_mode->mode.cursor_move = cursor_move;
}
