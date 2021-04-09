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
#include <hikari/renderer.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

static struct hikari_group_assign_mode *
get_mode(void)
{
  struct hikari_group_assign_mode *mode = &hikari_server.group_assign_mode;

  assert(mode == (struct hikari_group_assign_mode *)hikari_server.mode);

  return mode;
}

static void
init_completion(void)
{
  struct hikari_group_assign_mode *mode = get_mode();

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
  struct hikari_group_assign_mode *mode = get_mode();

  if (mode->completion == NULL) {
    return;
  }

  hikari_completion_fini(mode->completion);
  hikari_free(mode->completion);
  mode->completion = NULL;
}

static void
put_char(struct hikari_input_buffer *input_buffer,
    struct hikari_keyboard *keyboard,
    uint32_t keycode)
{
  uint32_t codepoint = hikari_keyboard_get_codepoint(keyboard, keycode);

  if (codepoint) {
    fini_completion();
    hikari_input_buffer_add_utf32_char(input_buffer, codepoint);
  }
}

static void
confirm_group_assign(void)
{
  struct hikari_group_assign_mode *mode = get_mode();
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *view = workspace->focus_view;
  struct hikari_group *group;

  char *input = mode->input_buffer.buffer;
  if (!strcmp(input, "")) {
    group = hikari_server_find_or_create_group(view->id);
  } else {
    group = hikari_server_find_or_create_group(input);
  }

  assert(group != NULL);

  hikari_view_group(view, group);

  hikari_server_enter_normal_mode(NULL);
}

static void
update_state(void)
{
  struct hikari_group_assign_mode *mode = get_mode();
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *view = workspace->focus_view;
  struct hikari_output *output = view->output;
  struct hikari_indicator *indicator = &hikari_server.indicator;

  struct wlr_box *geometry = hikari_view_border_geometry(view);
  char *input = mode->input_buffer.buffer;

  struct hikari_group *group = hikari_server_find_group(input);

  hikari_indicator_damage_group(indicator, output, geometry);

  if (!strcmp(input, "")) {
    hikari_indicator_update_group(indicator, output, " ");
  } else {
    hikari_indicator_update_group(indicator, output, input);
  }

  if (mode->group != group) {
    if (mode->group != NULL) {
      hikari_group_damage(mode->group);
    }

    if (group != NULL) {
      hikari_group_damage(group);
    }

    mode->group = group;
  }

  hikari_indicator_damage_group(indicator, output, geometry);
}

static void
handle_keysym(
    struct hikari_keyboard *keyboard, uint32_t keycode, xkb_keysym_t sym)
{
  struct hikari_group_assign_mode *mode = get_mode();
  struct hikari_input_buffer *input_buffer = &mode->input_buffer;

  char *text;

  switch (sym) {
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
      goto done;

    case XKB_KEY_e:
      if (hikari_keyboard_check_modifier(keyboard, WLR_MODIFIER_CTRL)) {
        if (mode->completion != NULL) {
          text = hikari_completion_cancel(mode->completion);
          hikari_input_buffer_replace(input_buffer, text);
          fini_completion();
        }
      } else {
        put_char(input_buffer, keyboard, keycode);
      }
      break;

    case XKB_KEY_h:
      fini_completion();
      if (hikari_keyboard_check_modifier(keyboard, WLR_MODIFIER_CTRL)) {
        hikari_input_buffer_remove_char(input_buffer);
      } else {
        put_char(input_buffer, keyboard, keycode);
      }
      break;

    case XKB_KEY_u:
      fini_completion();
      if (hikari_keyboard_check_modifier(keyboard, WLR_MODIFIER_CTRL)) {
        hikari_input_buffer_clear(input_buffer);
      } else {
        put_char(input_buffer, keyboard, keycode);
      }
      break;

    case XKB_KEY_w:
      fini_completion();
      if (hikari_keyboard_check_modifier(keyboard, WLR_MODIFIER_CTRL)) {
        hikari_input_buffer_remove_word(input_buffer);
      } else {
        put_char(input_buffer, keyboard, keycode);
      }
      break;

    case XKB_KEY_BackSpace:
      fini_completion();
      hikari_input_buffer_remove_char(input_buffer);
      break;

    case XKB_KEY_Tab:
      init_completion();
      text = hikari_completion_next(mode->completion);
      hikari_input_buffer_replace(input_buffer, text);
      break;

    case XKB_KEY_ISO_Left_Tab:
      init_completion();
      text = hikari_completion_prev(mode->completion);
      hikari_input_buffer_replace(input_buffer, text);
      break;

    case XKB_KEY_c:
    case XKB_KEY_d:
      if (!hikari_keyboard_check_modifier(keyboard, WLR_MODIFIER_CTRL)) {
        put_char(input_buffer, keyboard, keycode);
        break;
      }
    case XKB_KEY_Escape:
      hikari_server_enter_normal_mode(NULL);
      goto done;

    case XKB_KEY_Return:
      confirm_group_assign();
      goto done;

    default:
      put_char(input_buffer, keyboard, keycode);
      break;
  }

  update_state();

done:
  return;
}

static void
key_handler(
    struct hikari_keyboard *keyboard, struct wlr_event_keyboard_key *event)
{
  if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    uint32_t keycode = event->keycode + 8;
    hikari_keyboard_for_keysym(keyboard, keycode, handle_keysym);
  }
}

static void
modifiers_handler(struct hikari_keyboard *keyboard)
{}

static void
cancel(void)
{
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *view = workspace->focus_view;
  struct hikari_group_assign_mode *mode = get_mode();
  struct hikari_indicator *indicator = &hikari_server.indicator;

  hikari_indicator_set_color_group(
      indicator, hikari_configuration->indicator_selected);

  if (view != NULL) {
    struct hikari_output *output = view->output;
    struct hikari_indicator *indicator = &hikari_server.indicator;

    hikari_indicator_damage(indicator, view);

    hikari_indicator_update_group(indicator, output, view->group->name);

    hikari_view_damage_border(view);
  }

  if (mode->group != NULL) {
    hikari_group_damage(mode->group);
    mode->group = NULL;
  }

  hikari_input_buffer_clear(&mode->input_buffer);
  fini_completion();
}

static void
button_handler(
    struct hikari_cursor *cursor, struct wlr_event_pointer_button *event)
{}

static void
cursor_move(uint32_t time_msec)
{}

void
hikari_group_assign_mode_init(
    struct hikari_group_assign_mode *group_assign_mode)
{
  group_assign_mode->mode.key_handler = key_handler;
  group_assign_mode->mode.button_handler = button_handler;
  group_assign_mode->mode.modifiers_handler = modifiers_handler;
  group_assign_mode->mode.render = hikari_renderer_group_assign_mode;
  group_assign_mode->mode.cancel = cancel;
  group_assign_mode->mode.cursor_move = cursor_move;
  group_assign_mode->group = NULL;
  group_assign_mode->completion = NULL;
}

void
hikari_group_assign_mode_enter(struct hikari_view *view)
{
  struct hikari_group_assign_mode *mode = &hikari_server.group_assign_mode;

  struct hikari_output *output = hikari_server.workspace->output;
  struct hikari_indicator *indicator = &hikari_server.indicator;
  struct wlr_box *geometry = hikari_view_border_geometry(view);

  mode->group = view->group;
  hikari_server.mode = (struct hikari_mode *)mode;

  hikari_input_buffer_replace(&mode->input_buffer, view->group->name);

  hikari_indicator_set_color_group(
      indicator, hikari_configuration->indicator_insert);

  hikari_indicator_update_group(indicator, output, view->group->name);
  hikari_indicator_damage_group(indicator, output, geometry);
}
