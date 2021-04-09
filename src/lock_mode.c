#include <hikari/lock_mode.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <wlr/types/wlr_seat.h>

#include <hikari/cursor.h>
#include <hikari/keyboard.h>
#include <hikari/lock_indicator.h>
#include <hikari/output.h>
#include <hikari/renderer.h>
#include <hikari/server.h>
#include <hikari/utf8.h>
#include <hikari/view.h>

#define BUFFER_SIZE 1024

static char input_buffer[BUFFER_SIZE];
static int cursor = 0;
static int locker_pipe[2][2] = { { -1, -1 }, { -1, -1 } };

static struct hikari_lock_mode *
get_mode(void)
{
  struct hikari_lock_mode *mode = &hikari_server.lock_mode;

  assert(mode == (struct hikari_lock_mode *)hikari_server.mode);

  return mode;
}

static void
clear_buffer(void)
{
  cursor = 0;
  memset(input_buffer, 0, BUFFER_SIZE);
}

static void
clear_password(void)
{
  struct hikari_lock_mode *mode = get_mode();

  clear_buffer();
  hikari_lock_indicator_clear(mode->lock_indicator);
}

static void
start_unlocker(void)
{
  pipe(locker_pipe[0]);
  pipe(locker_pipe[1]);

  pid_t locker = fork();

  if (locker == 0) {
    close(locker_pipe[0][1]);
    close(locker_pipe[1][0]);
    close(0);
    close(1);
    dup2(locker_pipe[0][0], 0);
    dup2(locker_pipe[1][1], 1);
    execl("/bin/sh", "/bin/sh", "-c", "hikari-unlocker", NULL);
    exit(0);
  } else {
    close(locker_pipe[0][0]);
    close(locker_pipe[1][1]);
  }
}

static void
put_char(uint32_t codepoint)
{
  size_t length = utf8_chsize(codepoint);

  if (cursor + length < BUFFER_SIZE) {
    struct hikari_lock_mode *mode = get_mode();

    hikari_lock_indicator_set_type(mode->lock_indicator);
    utf8_encode(&input_buffer[cursor], length, codepoint);
    cursor += length;
  }
}

static void
delete_char(void)
{
  struct hikari_lock_mode *mode = get_mode();

  if (cursor == 0) {
    return;
  }

  hikari_lock_indicator_set_type(mode->lock_indicator);

  input_buffer[--cursor] = '\0';

  if (cursor == 0) {
    hikari_lock_indicator_clear(mode->lock_indicator);
  }
}

static void
submit_password(void)
{
  struct hikari_lock_mode *mode = get_mode();
  size_t password_length = strnlen(input_buffer, 1023) + 1;
  bool success = false;

  hikari_lock_indicator_set_verify(mode->lock_indicator);
  write(locker_pipe[0][1], input_buffer, password_length);
  clear_buffer();
  read(locker_pipe[1][0], &success, sizeof(bool));

  if (success) {
    int status;
    close(locker_pipe[1][0]);
    close(locker_pipe[0][1]);
    wait(&status);

    hikari_server_enter_normal_mode(NULL);
  } else {

    hikari_lock_indicator_set_deny(mode->lock_indicator);
  }
}

static void
disable_outputs(void)
{
  struct hikari_lock_mode *mode = get_mode();

  wl_event_source_timer_update(mode->disable_outputs, 0);

  struct hikari_output *output;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    hikari_output_disable(output);

    hikari_output_damage_whole(output);
  }

  mode->outputs_disabled = true;

  clear_password();
}

static void
enable_outputs(void)
{
  struct hikari_lock_mode *mode = get_mode();

  if (!mode->outputs_disabled) {
    return;
  }

  assert(cursor == 0);

  struct hikari_output *output;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    hikari_output_enable(output);
  }

  mode->outputs_disabled = false;
}

static void
key_handler(
    struct hikari_keyboard *keyboard, struct wlr_event_keyboard_key *event)
{
  struct hikari_lock_mode *mode = get_mode();

  if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    const xkb_keysym_t *syms;
    uint32_t keycode = event->keycode + 8;
    uint32_t codepoint;

    int nsyms = xkb_state_key_get_syms(
        keyboard->device->keyboard->xkb_state, keycode, &syms);

    enable_outputs();

    for (int i = 0; i < nsyms; i++) {
      switch (syms[i]) {
        case XKB_KEY_Caps_Lock:
        case XKB_KEY_Shift_L:
        case XKB_KEY_Shift_R:
        case XKB_KEY_Control_L:
        case XKB_KEY_Control_R:
        case XKB_KEY_Meta_L:
        case XKB_KEY_Meta_R:
        case XKB_KEY_Alt_L:
        case XKB_KEY_Alt_R:
        case XKB_KEY_Super_L:
        case XKB_KEY_Super_R:
          break;

        case XKB_KEY_Escape:
          clear_password();
          break;

        case XKB_KEY_BackSpace:
          delete_char();
          break;

        case XKB_KEY_Return:
          submit_password();
          break;

        case XKB_KEY_c:
          if (hikari_keyboard_check_modifier(keyboard, WLR_MODIFIER_CTRL)) {
            disable_outputs();
            return;
          }
        default:
          codepoint = hikari_keyboard_get_codepoint(keyboard, keycode);

          if (codepoint) {
            put_char(codepoint);
          }
          break;
      }
    }

    if (mode->disable_outputs != NULL) {
      wl_event_source_timer_update(mode->disable_outputs, 10 * 1000);
    }
  }
}

static void
modifiers_handler(struct hikari_keyboard *keyboard)
{}

static void
button_handler(
    struct hikari_cursor *cursor, struct wlr_event_pointer_button *event)
{}

static void
reset_visibility(void)
{
  struct hikari_output *output;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    struct hikari_view *view;
    wl_list_for_each (view, &output->views, output_views) {
      if (hikari_view_is_forced(view)) {
        hikari_view_unset_forced(view);

        if (hikari_view_is_hidden(view)) {
          hikari_view_unset_hidden(view);
        } else {
          hikari_view_set_hidden(view);
        }
      }
    }
  }
}

static void
cancel(void)
{
  struct hikari_lock_mode *mode = get_mode();

  wl_event_source_remove(mode->disable_outputs);
  mode->disable_outputs = NULL;

  struct hikari_output *output;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    hikari_output_enable(output);
    hikari_output_damage_whole(output);
  }

  hikari_lock_indicator_fini(mode->lock_indicator);
  hikari_free(mode->lock_indicator);
  mode->lock_indicator = NULL;

  reset_visibility();

  hikari_cursor_activate(&hikari_server.cursor);
}

static void
cursor_move(uint32_t time_msec)
{}

static int
disable_outputs_handler(void *data)
{
  assert(hikari_server_in_lock_mode());

  disable_outputs();

  return 0;
}

void
hikari_lock_mode_init(struct hikari_lock_mode *lock_mode)
{
  lock_mode->mode.key_handler = key_handler;
  lock_mode->mode.button_handler = button_handler;
  lock_mode->mode.modifiers_handler = modifiers_handler;
  lock_mode->mode.render = hikari_renderer_lock_mode;
  lock_mode->mode.cancel = cancel;
  lock_mode->mode.cursor_move = cursor_move;

  lock_mode->lock_indicator = NULL;

  mlock(input_buffer, BUFFER_SIZE);
  clear_buffer();
}

void
hikari_lock_mode_fini(struct hikari_lock_mode *lock_mode)
{
  munlock(input_buffer, BUFFER_SIZE);
}

static void
override_visibility(void)
{
  struct hikari_output *output;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    struct hikari_view *view;
    wl_list_for_each (view, &output->views, output_views) {
      if (hikari_view_is_public(view)) {
        if (hikari_view_is_hidden(view)) {
          hikari_view_set_forced(view);
          hikari_view_unset_hidden(view);
        }
      } else {
        if (!hikari_view_is_hidden(view)) {
          hikari_view_set_forced(view);
          hikari_view_set_hidden(view);
        }
      }
    }
  }
}

void
hikari_lock_mode_enter(void)
{
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *focus_view = workspace->focus_view;

#ifdef HAVE_LAYERSHELL
  struct hikari_layer *focus_layer = workspace->focus_layer;

  if (focus_layer != NULL) {
    assert(focus_view == NULL);

    struct wlr_seat *wlr_seat = hikari_server.seat;

    workspace->focus_layer = NULL;

    wlr_seat_keyboard_end_grab(wlr_seat);

    wlr_seat_pointer_clear_focus(wlr_seat);
    wlr_seat_keyboard_clear_focus(wlr_seat);
  } else if (focus_view != NULL) {
    assert(focus_layer == NULL);
    hikari_workspace_focus_view(workspace, NULL);
  }
#else
  if (focus_view != NULL) {
    hikari_workspace_focus_view(workspace, NULL);
  }
#endif

  hikari_cursor_deactivate(&hikari_server.cursor);

  hikari_server.mode = (struct hikari_mode *)&hikari_server.lock_mode;

  struct hikari_lock_mode *mode = get_mode();

  assert(mode->lock_indicator == NULL);

  mode->lock_indicator = hikari_malloc(sizeof(struct hikari_lock_indicator));

  hikari_lock_indicator_init(mode->lock_indicator);

  clear_buffer();
  start_unlocker();
  override_visibility();

  mode->disable_outputs = wl_event_loop_add_timer(
      hikari_server.event_loop, disable_outputs_handler, NULL);

  struct hikari_output *output;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    hikari_output_damage_whole(output);
  }

  wl_event_source_timer_update(mode->disable_outputs, 1000);
}
