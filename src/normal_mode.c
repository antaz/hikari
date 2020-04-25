#include <hikari/normal_mode.h>

#include <wlr/types/wlr_seat.h>

#include <hikari/action.h>
#include <hikari/bindings.h>
#include <hikari/configuration.h>
#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/keybinding.h>
#include <hikari/keyboard.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/view.h>

#ifndef NDEBUG
#include <hikari/layout.h>
#include <hikari/sheet.h>
#include <hikari/workspace.h>
#endif

static bool
handle_input(struct hikari_modifier_bindings *map, uint32_t code)
{
  int nbindings = map->nbindings;
  struct hikari_keybinding *bindings = map->bindings;

  for (int i = 0; i < nbindings; i++) {
    struct hikari_keybinding *binding = &bindings[i];

    if (binding->keycode == code) {
      struct hikari_event_action *event_action;

      if (binding->action->end.action != NULL) {
        hikari_server.normal_mode.pending_action = &binding->action->end;
      }

      event_action = &binding->action->begin;
      if (event_action->action != NULL) {
        event_action->action(event_action->arg);
      }
      return true;
    }
  }

  return false;
}

static bool
handle_pending_action(void)
{
  struct hikari_event_action *pending_action =
      hikari_server.normal_mode.pending_action;

  if (pending_action != NULL) {
    pending_action->action(pending_action->arg);
    hikari_server.normal_mode.pending_action = NULL;
    return true;
  }

  return false;
}

static void
normal_mode_keyboard_handler(struct hikari_workspace *workspace,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  if (handle_pending_action()) {
    return;
  }

  if (event->state == WLR_KEY_PRESSED) {
    uint32_t modifiers = hikari_server.keyboard_state.modifiers;
    struct hikari_modifier_bindings *map =
        &hikari_configuration->bindings.keyboard[modifiers];

    if (handle_input(map, event->keycode)) {
      return;
    }
  }

  wlr_seat_set_keyboard(hikari_server.seat, keyboard->device);
  wlr_seat_keyboard_notify_key(
      hikari_server.seat, event->time_msec, event->keycode, event->state);
}

static void
normal_mode_button_handler(
    struct hikari_workspace *workspace, struct wlr_event_pointer_button *event)
{
  if (handle_pending_action()) {
    return;
  }

  if (event->state == WLR_BUTTON_PRESSED) {
    uint32_t modifiers = hikari_server.keyboard_state.modifiers;
    struct hikari_modifier_bindings *map =
        &hikari_configuration->bindings.mouse[modifiers];

    if (handle_input(map, event->button)) {
      return;
    }
  }

  wlr_seat_pointer_notify_button(
      hikari_server.seat, event->time_msec, event->button, event->state);
}

static void
button_handler(struct wl_listener *listener, void *data)
{
  struct wlr_event_pointer_button *event = data;
  struct hikari_workspace *workspace = hikari_server.workspace;

  normal_mode_button_handler(workspace, event);
}

static void
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_event_keyboard_key *event = data;

  struct hikari_workspace *workspace = hikari_server.workspace;

  normal_mode_keyboard_handler(workspace, event, keyboard);
}

#ifndef NDEBUG
static void
dump_debug(struct hikari_server *server)
{
  printf("---------------------------------------------------------------------"
         "\n");
  printf("GROUPS\n");
  printf("---------------------------------------------------------------------"
         "\n");
  struct hikari_group *group = NULL;
  struct hikari_view *view;
  wl_list_for_each (
      group, &hikari_server.visible_groups, visible_server_groups) {
    printf("%s ", group->name);
    view = NULL;
    wl_list_for_each (view, &group->views, group_views) {
      printf("%p ", view);
    }

    printf("/ ");

    view = NULL;
    wl_list_for_each (view, &group->visible_views, visible_group_views) {
      printf("%p ", view);
    }

    printf("\n");
  }
  printf("---------------------------------------------------------------------"
         "\n");
  printf("SHEETS\n");
  printf("---------------------------------------------------------------------"
         "\n");
  struct hikari_sheet *sheets = hikari_server.workspace->sheets;
  struct hikari_sheet *sheet;
  for (int i = 0; i < 10; i++) {
    sheet = &sheets[i];
    printf("%d ", sheet->nr);
    view = NULL;
    wl_list_for_each (view, &sheet->views, sheet_views) {
      printf("%p ", view);
    }
    printf("\n");
  }
  printf("---------------------------------------------------------------------"
         "\n");
  if (hikari_server.workspace->sheet->layout != NULL) {
    printf("-------------------------------------------------------------------"
           "--\n");
    printf("LAYOUT\n");
    struct hikari_tile *tile;
    wl_list_for_each (
        tile, &hikari_server.workspace->sheet->layout->tiles, layout_tiles) {
      printf("%p ", tile->view);
    }
    printf("\n");
    printf("-------------------------------------------------------------------"
           "--\n");
  }
}
#endif

static void
modifier_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard =
      wl_container_of(listener, keyboard, modifiers);

  wlr_seat_set_keyboard(hikari_server.seat, keyboard->device);
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (hikari_server.keyboard_state.mod_released) {

    if (hikari_server_is_cycling() && focus_view != NULL) {
      hikari_view_raise(focus_view);
      hikari_view_center_cursor(focus_view);
    }

    hikari_server_unset_cycling();
  }

  if (hikari_server.keyboard_state.mod_changed) {
    if (focus_view != NULL) {
      hikari_group_damage(focus_view->group);
      hikari_indicator_damage(&hikari_server.indicator, focus_view);
    }
#ifndef NDEBUG
    dump_debug(&hikari_server);
#endif
  }

  wlr_seat_keyboard_notify_modifiers(
      hikari_server.seat, &keyboard->device->keyboard->modifiers);
}

static void
render(struct hikari_output *output, struct hikari_render_data *render_data)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (hikari_server.keyboard_state.mod_pressed && focus_view != NULL) {
    struct hikari_workspace *workspace = hikari_server.workspace;
    struct hikari_group *group = focus_view->group;

    float *indicator_first = hikari_configuration->indicator_first;
    float *indicator_grouped = hikari_configuration->indicator_grouped;
    struct hikari_view *view;
    wl_list_for_each_reverse (
        view, &group->visible_views, visible_group_views) {
      if (view != focus_view && view->output == output) {
        render_data->geometry = hikari_view_border_geometry(view);

        if (hikari_group_first_view(group, workspace) == view) {
          hikari_indicator_frame_render(
              &view->indicator_frame, indicator_first, render_data);
        } else {
          hikari_indicator_frame_render(
              &view->indicator_frame, indicator_grouped, render_data);
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
}

static void
cancel(void)
{}

void
hikari_normal_mode_init(struct hikari_normal_mode *normal_mode)
{
  normal_mode->mode.key_handler = key_handler;
  normal_mode->mode.button_handler = button_handler;
  normal_mode->mode.modifier_handler = modifier_handler;
  normal_mode->mode.render = render;
  normal_mode->mode.cancel = cancel;
  normal_mode->mode.cursor_move = NULL;
  normal_mode->pending_action = NULL;
}

void
hikari_normal_mode_enter(void)
{
  struct hikari_server *server = &hikari_server;

  assert(server->workspace != NULL);

  server->mode->cancel();
  server->mode = (struct hikari_mode *)&server->normal_mode;
}
