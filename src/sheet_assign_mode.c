#include <hikari/sheet_assign_mode.h>

#include <wayland-util.h>

#include <hikari/configuration.h>
#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/normal_mode.h>
#include <hikari/renderer.h>
#include <hikari/sheet.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

static struct hikari_sheet_assign_mode *
get_mode(void)
{
  struct hikari_sheet_assign_mode *mode = &hikari_server.sheet_assign_mode;

  assert(mode == (struct hikari_sheet_assign_mode *)hikari_server.mode);

  return mode;
}

#define CYCLE_SHEET(name)                                                      \
  static struct hikari_sheet *name##_sheet(struct hikari_sheet *sheet)         \
  {                                                                            \
    assert(sheet != NULL);                                                     \
                                                                               \
    struct hikari_workspace *name = hikari_workspace_##name(sheet->workspace); \
                                                                               \
    assert(name != NULL);                                                      \
                                                                               \
    return &name->sheets[sheet->nr];                                           \
  }

CYCLE_SHEET(next)
CYCLE_SHEET(prev)
#undef CYCLE_SHEET

static void
confirm_sheet_assign(struct hikari_workspace *workspace)
{
  struct hikari_sheet_assign_mode *mode = get_mode();
  struct hikari_view *focus_view = workspace->focus_view;
  struct hikari_sheet *sheet = mode->sheet;
  struct hikari_output *view_output = focus_view->output;
  struct hikari_output *sheet_output = sheet->workspace->output;

  assert(sheet != NULL);

  if (sheet_output == view_output) {
    hikari_view_pin_to_sheet(focus_view, sheet);
  } else {
    int lx = sheet_output->geometry.x + focus_view->geometry.x;
    int ly = sheet_output->geometry.y + focus_view->geometry.y;
    hikari_server_migrate_focus_view(sheet_output, lx, ly, true);
  }

  hikari_server_enter_normal_mode(NULL);
}

static void
update_state(struct hikari_workspace *workspace, struct hikari_sheet *sheet)
{
  struct hikari_sheet_assign_mode *mode = get_mode();
  struct hikari_view *view = workspace->focus_view;
  struct hikari_output *output = view->output;
  struct hikari_indicator *indicator = &hikari_server.indicator;
  struct wlr_box *geometry = hikari_view_border_geometry(view);

  assert(sheet != NULL);

  hikari_indicator_damage_sheet(indicator, output, geometry);

  hikari_indicator_update_sheet(indicator, output, sheet, view->flags);

  hikari_indicator_damage_sheet(indicator, output, geometry);

  mode->sheet = sheet;
}

static void
handle_keysym(
    struct hikari_keyboard *keyboard, uint32_t keycode, xkb_keysym_t sym)
{
  struct hikari_sheet_assign_mode *mode = get_mode();
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_sheet *sheet = mode->sheet;

  assert(sheet != NULL);

  switch (sym) {
    case XKB_KEY_1:
      sheet = &workspace->sheets[1];
      break;

    case XKB_KEY_2:
      sheet = &workspace->sheets[2];
      break;

    case XKB_KEY_3:
      sheet = &workspace->sheets[3];
      break;

    case XKB_KEY_4:
      sheet = &workspace->sheets[4];
      break;

    case XKB_KEY_5:
      sheet = &workspace->sheets[5];
      break;

    case XKB_KEY_6:
      sheet = &workspace->sheets[6];
      break;

    case XKB_KEY_7:
      sheet = &workspace->sheets[7];
      break;

    case XKB_KEY_8:
      sheet = &workspace->sheets[8];
      break;

    case XKB_KEY_9:
      sheet = &workspace->sheets[9];
      break;

    case XKB_KEY_0:
      sheet = &workspace->sheets[0];
      break;

    case XKB_KEY_Tab:
      sheet = next_sheet(sheet);
      break;

    case XKB_KEY_ISO_Left_Tab:
      sheet = prev_sheet(sheet);
      break;

    case XKB_KEY_c:
    case XKB_KEY_d:
      if (!hikari_keyboard_check_modifier(keyboard, WLR_MODIFIER_CTRL)) {
        goto done;
      }
    case XKB_KEY_Escape:
      hikari_server_enter_normal_mode(workspace);
      goto done;

    case XKB_KEY_Return:
      confirm_sheet_assign(workspace);
      goto done;

    default:
      goto done;
  }

  update_state(workspace, sheet);

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
  struct hikari_sheet_assign_mode *mode = get_mode();

  if (view != NULL) {
    struct hikari_output *output = view->output;
    struct hikari_indicator *indicator = &hikari_server.indicator;

    hikari_indicator_damage(indicator, view);
    hikari_indicator_set_color_sheet(
        indicator, hikari_configuration->indicator_selected);
    hikari_indicator_update_sheet(indicator, output, view->sheet, view->flags);
    hikari_view_damage_border(view);
  }

  mode->sheet = NULL;
}

static void
button_handler(
    struct hikari_cursor *cursor, struct wlr_event_pointer_button *event)
{}

static void
cursor_move(uint32_t time_msec)
{}

void
hikari_sheet_assign_mode_init(
    struct hikari_sheet_assign_mode *sheet_assign_mode)
{
  sheet_assign_mode->mode.key_handler = key_handler;
  sheet_assign_mode->mode.button_handler = button_handler;
  sheet_assign_mode->mode.modifiers_handler = modifiers_handler;
  sheet_assign_mode->mode.render = hikari_renderer_sheet_assign_mode;
  sheet_assign_mode->mode.cancel = cancel;
  sheet_assign_mode->mode.cursor_move = cursor_move;
  sheet_assign_mode->sheet = NULL;
}

void
hikari_sheet_assign_mode_enter(struct hikari_view *view)
{
  struct hikari_output *output = hikari_server.workspace->output;
  struct hikari_indicator *indicator = &hikari_server.indicator;
  struct wlr_box *geometry = hikari_view_border_geometry(view);

  hikari_server.sheet_assign_mode.sheet = view->sheet;
  hikari_server.mode = (struct hikari_mode *)&hikari_server.sheet_assign_mode;

  hikari_indicator_set_color_sheet(
      indicator, hikari_configuration->indicator_insert);
  hikari_indicator_update_sheet(
      indicator, view->output, view->sheet, view->flags);
  hikari_indicator_damage_sheet(indicator, output, geometry);
}
