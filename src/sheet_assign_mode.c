#include <hikari/sheet_assign_mode.h>

#include <wayland-util.h>

#include <hikari/configuration.h>
#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/normal_mode.h>
#include <hikari/render_data.h>
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
  struct wlr_box *geometry = hikari_view_geometry(focus_view);
  struct hikari_sheet *sheet = mode->sheet;

  assert(sheet != NULL);

  hikari_indicator_update_sheet(&hikari_server.indicator,
      geometry,
      workspace->output,
      sheet,
      hikari_configuration->indicator_selected,
      hikari_view_is_invisible(focus_view),
      hikari_view_is_floating(focus_view));

  hikari_view_pin_to_sheet(focus_view, sheet);

  hikari_server_enter_normal_mode(NULL);
}

static void
cancel_sheet_assign(struct hikari_workspace *workspace)
{
  struct hikari_view *focus_view = workspace->focus_view;
  struct wlr_box *geometry = hikari_view_geometry(focus_view);

  assert(focus_view->sheet != NULL);

  hikari_indicator_update_sheet(&hikari_server.indicator,
      geometry,
      workspace->output,
      focus_view->sheet,
      hikari_configuration->indicator_selected,
      hikari_view_is_invisible(focus_view),
      hikari_view_is_floating(focus_view));

  hikari_server_enter_normal_mode(NULL);
}

static void
update_state(struct hikari_workspace *workspace, struct hikari_sheet *sheet)
{
  struct hikari_sheet_assign_mode *mode = get_mode();
  struct hikari_view *focus_view = workspace->focus_view;
  struct wlr_box *geometry = hikari_view_border_geometry(focus_view);

  assert(sheet != NULL);

  hikari_indicator_update_sheet(&hikari_server.indicator,
      geometry,
      workspace->output,
      sheet,
      hikari_configuration->indicator_insert,
      hikari_view_is_invisible(focus_view),
      hikari_view_is_floating(focus_view));

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
      cancel_sheet_assign(workspace);
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
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_event_keyboard_key *event = data;

  if (event->state == WLR_KEY_PRESSED) {
    uint32_t keycode = event->keycode + 8;
    hikari_keyboard_for_keysym(keyboard, keycode, handle_keysym);
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
  hikari_server.sheet_assign_mode.sheet = NULL;
}

static void
button_handler(struct wl_listener *listener, void *data)
{}

static void
cursor_move(void)
{}

void
hikari_sheet_assign_mode_init(
    struct hikari_sheet_assign_mode *sheet_assign_mode)
{
  sheet_assign_mode->mode.key_handler = key_handler;
  sheet_assign_mode->mode.button_handler = button_handler;
  sheet_assign_mode->mode.modifier_handler = modifier_handler;
  sheet_assign_mode->mode.render = render;
  sheet_assign_mode->mode.cancel = cancel;
  sheet_assign_mode->mode.cursor_move = cursor_move;
  sheet_assign_mode->sheet = NULL;
}
