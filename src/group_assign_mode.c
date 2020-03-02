#include <hikari/group_assign_mode.h>

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <wayland-util.h>

#include <hikari/color.h>
#include <hikari/completion.h>
#include <hikari/configuration.h>
#include <hikari/group.h>
#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/memory.h>
#include <hikari/normal_mode.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

static void
init_completion(void)
{
  struct hikari_group_assign_mode *mode =
      (struct hikari_group_assign_mode *)hikari_server.mode;

  assert(mode == (struct hikari_group_assign_mode *)hikari_server.mode);

  if (mode->completion != NULL) {
    return;
  }

  struct hikari_group *group;
  struct hikari_completion *completion =
      hikari_malloc(sizeof(struct hikari_completion));

  char *input = mode->input_buffer.buffer;

  hikari_completion_init(completion, input);

  wl_list_for_each (group, &hikari_server.groups, server_groups) {
    char *name = group->name;

    if (strstr(name, input) == name && strcmp(name, input) != 0) {
      hikari_completion_add(completion, name);
    }
  }

  mode->completion = completion;
}

static void
fini_completion(void)
{
  struct hikari_group_assign_mode *mode =
      (struct hikari_group_assign_mode *)hikari_server.mode;

  assert(mode == (struct hikari_group_assign_mode *)hikari_server.mode);

  if (mode->completion == NULL) {
    return;
  }

  hikari_completion_fini(mode->completion);
  hikari_free(mode->completion);
  mode->completion = NULL;
}

static void
put_char(struct hikari_input_buffer *input_buffer,
    struct xkb_state *xkb_state,
    uint32_t keycode)
{
  uint32_t codepoint = xkb_state_key_get_utf32(xkb_state, keycode);

  if (codepoint) {
    fini_completion();
    hikari_input_buffer_add_utf32_char(input_buffer, codepoint);
  }
}

static void
process_input_key(struct hikari_workspace *workspace,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  struct hikari_group_assign_mode *mode =
      (struct hikari_group_assign_mode *)hikari_server.mode;

  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);

  const xkb_keysym_t *syms;
  uint32_t keycode = event->keycode + 8;
  char *text;

  int nsyms = xkb_state_key_get_syms(
      keyboard->device->keyboard->xkb_state, keycode, &syms);

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

      case XKB_KEY_e:
        if (modifiers == WLR_MODIFIER_CTRL) {
          if (mode->completion != NULL) {
            text = hikari_completion_cancel(mode->completion);
            hikari_input_buffer_replace(&mode->input_buffer, text);
            fini_completion();
          }
        } else {
          put_char(&mode->input_buffer,
              keyboard->device->keyboard->xkb_state,
              keycode);
        }
        break;

      case XKB_KEY_h:
        fini_completion();
        if (modifiers == WLR_MODIFIER_CTRL) {
          hikari_input_buffer_remove_char(&mode->input_buffer);
        } else {
          put_char(&mode->input_buffer,
              keyboard->device->keyboard->xkb_state,
              keycode);
        }
        break;

      case XKB_KEY_w:
        fini_completion();
        if (modifiers == WLR_MODIFIER_CTRL) {
          hikari_input_buffer_clear(&mode->input_buffer);
        } else {
          put_char(&mode->input_buffer,
              keyboard->device->keyboard->xkb_state,
              keycode);
        }
        break;

      case XKB_KEY_BackSpace:
        fini_completion();
        hikari_input_buffer_remove_char(&mode->input_buffer);
        break;

      case XKB_KEY_Tab:
        init_completion();
        text = hikari_completion_next(mode->completion);
        hikari_input_buffer_replace(&mode->input_buffer, text);
        break;

      case XKB_KEY_ISO_Left_Tab:
        init_completion();
        text = hikari_completion_prev(mode->completion);
        hikari_input_buffer_replace(&mode->input_buffer, text);
        break;

      default:
        put_char(&mode->input_buffer,
            keyboard->device->keyboard->xkb_state,
            keycode);
        break;
    }
  }
}

static void
confirm_group_assign(struct hikari_workspace *workspace)
{
  struct hikari_group_assign_mode *mode =
      (struct hikari_group_assign_mode *)&hikari_server.group_assign_mode;

  assert(mode == (struct hikari_group_assign_mode *)hikari_server.mode);

  struct wlr_box *geometry = hikari_view_geometry(workspace->focus_view);

  struct hikari_group *group;
  char *input = mode->input_buffer.buffer;
  if (!strcmp(input, "")) {
    group = workspace->sheet->group;
    hikari_indicator_update_group(&hikari_server.indicator,
        geometry,
        workspace->output,
        "",
        hikari_configuration->indicator_selected);
  } else {
    if (isdigit(input[0])) {
      group = hikari_server_find_group(input);

      if (group == NULL) {
        hikari_indicator_update_group(&hikari_server.indicator,
            geometry,
            workspace->output,
            input,
            hikari_configuration->indicator_conflict);
        return;
      }
    } else {
      group = hikari_server_find_or_create_group(input);
    }

    assert(group != NULL);

    if (group->sheet == NULL) {
      hikari_indicator_update_group(&hikari_server.indicator,
          geometry,
          workspace->output,
          group->name,
          hikari_configuration->indicator_selected);
    } else {
      hikari_indicator_update_group(&hikari_server.indicator,
          geometry,
          workspace->output,
          "",
          hikari_configuration->indicator_selected);
    }
  }
  hikari_view_group(hikari_server.workspace->focus_view, group);
  hikari_server_enter_normal_mode(NULL);
}

static void
cancel_group_assign(struct hikari_workspace *workspace)
{
  struct hikari_view *focus_view = workspace->focus_view;
  struct wlr_box *geometry = hikari_view_geometry(focus_view);

  if (focus_view->group != focus_view->sheet->group) {
    hikari_indicator_update_group(&hikari_server.indicator,
        geometry,
        workspace->output,
        focus_view->group->name,
        hikari_configuration->indicator_selected);
  } else {
    hikari_indicator_update_group(&hikari_server.indicator,
        geometry,
        workspace->output,
        "",
        hikari_configuration->indicator_selected);
  }
  hikari_server_enter_normal_mode(NULL);
}

static void
update_state(struct hikari_workspace *workspace,
    struct hikari_keyboard *keyboard,
    struct wlr_event_keyboard_key *event)
{
  struct hikari_group_assign_mode *mode =
      (struct hikari_group_assign_mode *)&hikari_server.group_assign_mode;

  assert(mode == (struct hikari_group_assign_mode *)hikari_server.mode);

  struct wlr_box *geometry = hikari_view_border_geometry(workspace->focus_view);

  struct hikari_group *group;

  process_input_key(workspace, event, keyboard);
  group = hikari_server_find_group(mode->input_buffer.buffer);

  if (!strcmp(mode->input_buffer.buffer, "")) {
    hikari_indicator_update_group(&hikari_server.indicator,
        geometry,
        workspace->output,
        " ",
        hikari_configuration->indicator_insert);
  } else {
    hikari_indicator_update_group(&hikari_server.indicator,
        geometry,
        workspace->output,
        mode->input_buffer.buffer,
        hikari_configuration->indicator_insert);
  }

  if (group != mode->group) {
    mode->group = group;
    hikari_server_refresh_indication();
  }
}

static void
assign_group(struct hikari_workspace *workspace,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  if (hikari_keyboard_confirmation(keyboard, event)) {
    confirm_group_assign(workspace);
  } else if (hikari_keyboard_cancellation(keyboard, event)) {
    cancel_group_assign(workspace);
  } else {
    update_state(workspace, keyboard, event);
  }
}

static void
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_event_keyboard_key *event = data;

  struct hikari_workspace *workspace = hikari_server.workspace;

  if (event->state == WLR_KEY_PRESSED) {
    assign_group(workspace, event, keyboard);
  }
}

static void
modifier_handler(struct wl_listener *listener, void *data)
{}

static void
render(struct hikari_output *output, struct hikari_render_data *render_data)
{
  struct hikari_group_assign_mode *mode = &hikari_server.group_assign_mode;

  assert(mode == (struct hikari_group_assign_mode *)hikari_server.mode);

  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  assert(focus_view != NULL);

  struct hikari_group *group = mode->group;

  if (group != NULL) {
    float *indicator_first = hikari_configuration->indicator_first;
    float *indicator_grouped = hikari_configuration->indicator_grouped;
    struct hikari_view *view;
    wl_list_for_each_reverse (
        view, &group->visible_views, visible_group_views) {
      if (view->output == output && view != focus_view) {
        render_data->geometry = hikari_view_border_geometry(view);

        if (hikari_group_first_view(group, hikari_server.workspace) == view) {

          hikari_indicator_frame_render(
              &view->indicator_frame, indicator_first, render_data);
        } else {
          hikari_indicator_frame_render(
              &view->indicator_frame, indicator_grouped, render_data);
        }
      }
    }
  }

  if (focus_view->output == output) {
    render_data->geometry = hikari_view_border_geometry(focus_view);

    hikari_indicator_frame_render(&focus_view->indicator_frame,
        hikari_configuration->indicator_selected,
        render_data);

    hikari_indicator_render(&hikari_server.indicator, render_data);
  }
}

static void
cancel(void)
{
  hikari_input_buffer_clear(&hikari_server.group_assign_mode.input_buffer);
  fini_completion();
  hikari_server.group_assign_mode.group = NULL;
}

static void
button_handler(struct wl_listener *listener, void *data)
{}

static void
cursor_move(void)
{}

void
hikari_group_assign_mode_init(
    struct hikari_group_assign_mode *group_assign_mode)
{
  group_assign_mode->mode.type = HIKARI_MODE_TYPE_GROUP_CHANGE;
  group_assign_mode->mode.key_handler = key_handler;
  group_assign_mode->mode.button_handler = button_handler;
  group_assign_mode->mode.modifier_handler = modifier_handler;
  group_assign_mode->mode.render = render;
  group_assign_mode->mode.cancel = cancel;
  group_assign_mode->mode.cursor_move = cursor_move;
  group_assign_mode->group = NULL;
  group_assign_mode->completion = NULL;
}
