#include <hikari/mark_select_mode.h>

#include <stdbool.h>

#include <hikari/keyboard.h>
#include <hikari/mark.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/workspace.h>

static void
mark_select(struct hikari_workspace *workspace,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  struct hikari_mark *mark = hikari_keyboard_resolve_mark(keyboard, event);

  hikari_server_enter_normal_mode(NULL);

  if (mark != NULL) {
    if (hikari_server.mark_select_mode.switch_workspace) {
      hikari_server_switch_to_mark(mark);
    } else {
      hikari_server_show_mark(mark);
    }
  }
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
