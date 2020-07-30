#include <hikari/action.h>

#include <stdlib.h>

#include <hikari/action_config.h>
#include <hikari/configuration.h>
#include <hikari/server.h>

static char *
lookup_action(struct wl_list *action_configs, const char *action_name)
{
  struct hikari_action_config *action_config;
  wl_list_for_each (action_config, action_configs, link) {
    if (!strcmp(action_name, action_config->action_name)) {
      return action_config->command;
    }
  }

  return NULL;
}

static bool
parse_binding(struct wl_list *action_configs,
    const ucl_object_t *obj,
    void (**action)(void *),
    void **arg)
{
  const char *str;
  bool success = false;

  if (!ucl_object_tostring_safe(obj, &str)) {
    fprintf(
        stderr, "configuration error: expected string for binding action\n");
    goto done;
  }

  if (!strcmp(str, "quit")) {
    *action = hikari_server_terminate;
    *arg = NULL;
  } else if (!strcmp(str, "lock")) {
    *action = hikari_server_lock;
    *arg = NULL;
  } else if (!strcmp(str, "reload")) {
    *action = hikari_server_reload;
    *arg = NULL;
#ifndef NDEBUG
  } else if (!strcmp(str, "debug-damage")) {
    *action = hikari_server_toggle_damage_tracking;
    *arg = NULL;
#endif

#define PARSE_MOVE_BINDING(d, f)                                               \
  }                                                                            \
  else if (!strcmp(str, "view-move-" d))                                       \
  {                                                                            \
    *action = hikari_server_move_view_##f;                                     \
    *arg = NULL;

    PARSE_MOVE_BINDING("up", up)
    PARSE_MOVE_BINDING("down", down)
    PARSE_MOVE_BINDING("left", left)
    PARSE_MOVE_BINDING("right", right)
    PARSE_MOVE_BINDING("bottom-left", bottom_left)
    PARSE_MOVE_BINDING("bottom-middle", bottom_middle)
    PARSE_MOVE_BINDING("bottom-right", bottom_right)
    PARSE_MOVE_BINDING("center-left", center_left)
    PARSE_MOVE_BINDING("center", center)
    PARSE_MOVE_BINDING("center-right", center_right)
    PARSE_MOVE_BINDING("top-left", top_left)
    PARSE_MOVE_BINDING("top-middle", top_middle)
    PARSE_MOVE_BINDING("top-right", top_right)
#undef PARSE_MOVE_BINDING

#define PARSE_SNAP_BINDING(d)                                                  \
  }                                                                            \
  else if (!strcmp(str, "view-snap-" #d))                                      \
  {                                                                            \
    *action = hikari_server_snap_view_##d;                                     \
    *arg = NULL;

    PARSE_SNAP_BINDING(up)
    PARSE_SNAP_BINDING(down)
    PARSE_SNAP_BINDING(left)
    PARSE_SNAP_BINDING(right)
#undef PARSE_SNAP_BINDING

  } else if (!strcmp(str, "view-toggle-maximize-vertical")) {
    *action = hikari_server_toggle_view_vertical_maximize;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-maximize-horizontal")) {
    *action = hikari_server_toggle_view_horizontal_maximize;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-maximize-full")) {
    *action = hikari_server_toggle_view_full_maximize;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-floating")) {
    *action = hikari_server_toggle_view_floating;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-invisible")) {
    *action = hikari_server_toggle_view_invisible;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-public")) {
    *action = hikari_server_toggle_view_public;
    *arg = NULL;

  } else if (!strcmp(str, "view-raise")) {
    *action = hikari_server_raise_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-lower")) {
    *action = hikari_server_lower_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-hide")) {
    *action = hikari_server_hide_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-only")) {
    *action = hikari_server_only_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-quit")) {
    *action = hikari_server_quit_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-reset-geometry")) {
    *action = hikari_server_reset_view_geometry;
    *arg = NULL;
  } else if (!strcmp(str, "view-cycle-next")) {
    *action = hikari_server_cycle_next_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-cycle-prev")) {
    *action = hikari_server_cycle_prev_view;
    *arg = NULL;

#define PARSE_PIN_BINDING(n)                                                   \
  }                                                                            \
  else if (!strcmp(str, "view-pin-to-sheet-" #n))                              \
  {                                                                            \
    *action = hikari_server_pin_view_to_sheet_##n;                             \
    *arg = NULL;

    PARSE_PIN_BINDING(0)
    PARSE_PIN_BINDING(1)
    PARSE_PIN_BINDING(2)
    PARSE_PIN_BINDING(3)
    PARSE_PIN_BINDING(4)
    PARSE_PIN_BINDING(5)
    PARSE_PIN_BINDING(6)
    PARSE_PIN_BINDING(7)
    PARSE_PIN_BINDING(8)
    PARSE_PIN_BINDING(9)
    PARSE_PIN_BINDING(alternate)
    PARSE_PIN_BINDING(current)
    PARSE_PIN_BINDING(next)
    PARSE_PIN_BINDING(prev)
#undef PARSE_PIN_BINDING

  } else if (!strcmp(str, "view-decrease-size-up")) {
    *action = hikari_server_decrease_view_size_up;
    *arg = NULL;
  } else if (!strcmp(str, "view-increase-size-up")) {
    *action = hikari_server_increase_view_size_up;
    *arg = NULL;
  } else if (!strcmp(str, "view-decrease-size-down")) {
    *action = hikari_server_decrease_view_size_down;
    *arg = NULL;
  } else if (!strcmp(str, "view-increase-size-down")) {
    *action = hikari_server_increase_view_size_down;
    *arg = NULL;
  } else if (!strcmp(str, "view-decrease-size-left")) {
    *action = hikari_server_decrease_view_size_left;
    *arg = NULL;
  } else if (!strcmp(str, "view-increase-size-left")) {
    *action = hikari_server_increase_view_size_left;
    *arg = NULL;
  } else if (!strcmp(str, "view-increase-size-right")) {
    *action = hikari_server_increase_view_size_right;
    *arg = NULL;
  } else if (!strcmp(str, "view-decrease-size-right")) {
    *action = hikari_server_decrease_view_size_right;
    *arg = NULL;

#define PARSE_WORKSPACE_BINDING(n)                                             \
  }                                                                            \
  else if (!strcmp(str, "workspace-switch-to-sheet-" #n))                      \
  {                                                                            \
    *action = hikari_server_display_sheet_##n;                                 \
    *arg = NULL;

    PARSE_WORKSPACE_BINDING(0)
    PARSE_WORKSPACE_BINDING(1)
    PARSE_WORKSPACE_BINDING(2)
    PARSE_WORKSPACE_BINDING(3)
    PARSE_WORKSPACE_BINDING(4)
    PARSE_WORKSPACE_BINDING(5)
    PARSE_WORKSPACE_BINDING(6)
    PARSE_WORKSPACE_BINDING(7)
    PARSE_WORKSPACE_BINDING(8)
    PARSE_WORKSPACE_BINDING(9)
    PARSE_WORKSPACE_BINDING(alternate)
    PARSE_WORKSPACE_BINDING(current)
    PARSE_WORKSPACE_BINDING(next)
    PARSE_WORKSPACE_BINDING(prev)
#undef PARSE_WORKSPACE_BINDING
  } else if (!strcmp(str, "workspace-switch-to-sheet-next-inhabited")) {
    *action = hikari_server_switch_to_next_inhabited_sheet;
    *arg = NULL;
  } else if (!strcmp(str, "workspace-switch-to-sheet-prev-inhabited")) {
    *action = hikari_server_switch_to_prev_inhabited_sheet;
    *arg = NULL;

  } else if (!strcmp(str, "sheet-show-invisible")) {
    *action = hikari_server_show_invisible_sheet_views;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-show-all")) {
    *action = hikari_server_show_all_sheet_views;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-show-group")) {
    *action = hikari_server_sheet_show_group;
    *arg = NULL;

  } else if (!strcmp(str, "workspace-clear")) {
    *action = hikari_server_clear;
    *arg = NULL;
  } else if (!strcmp(str, "workspace-show-all")) {
    *action = hikari_server_show_all;
    *arg = NULL;
  } else if (!strcmp(str, "workspace-show-group")) {
    *action = hikari_server_show_group;
    *arg = NULL;
  } else if (!strcmp(str, "workspace-show-invisible")) {
    *action = hikari_server_show_invisible;
    *arg = NULL;
  } else if (!strcmp(str, "workspace-cycle-next")) {
    *action = hikari_server_cycle_next_workspace;
    *arg = NULL;
  } else if (!strcmp(str, "workspace-cycle-prev")) {
    *action = hikari_server_cycle_prev_workspace;
    *arg = NULL;

  } else if (!strcmp(str, "layout-cycle-view-next")) {
    *action = hikari_server_cycle_next_layout_view;
    *arg = NULL;
  } else if (!strcmp(str, "layout-cycle-view-prev")) {
    *action = hikari_server_cycle_prev_layout_view;
    *arg = NULL;
  } else if (!strcmp(str, "layout-cycle-view-first")) {
    *action = hikari_server_cycle_first_layout_view;
    *arg = NULL;
  } else if (!strcmp(str, "layout-cycle-view-last")) {
    *action = hikari_server_cycle_last_layout_view;
    *arg = NULL;
  } else if (!strcmp(str, "layout-exchange-view-next")) {
    *action = hikari_server_exchange_next_view;
    *arg = NULL;
  } else if (!strcmp(str, "layout-exchange-view-prev")) {
    *action = hikari_server_exchange_prev_view;
    *arg = NULL;
  } else if (!strcmp(str, "layout-exchange-view-main")) {
    *action = hikari_server_exchange_main_layout_view;
    *arg = NULL;
  } else if (!strcmp(str, "layout-reset")) {
    *action = hikari_server_reset_sheet_layout;
    *arg = NULL;
  } else if (!strcmp(str, "layout-restack-append")) {
    *action = hikari_server_layout_restack_append;
    *arg = NULL;
  } else if (!strcmp(str, "layout-restack-prepend")) {
    *action = hikari_server_layout_restack_prepend;
    *arg = NULL;

  } else if (!strcmp(str, "mode-enter-group-assign")) {
    *action = hikari_server_enter_group_assign_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-input-grab")) {
    *action = hikari_server_enter_input_grab_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-mark-assign")) {
    *action = hikari_server_enter_mark_assign_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-mark-select")) {
    *action = hikari_server_enter_mark_select_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-mark-switch-select")) {
    *action = hikari_server_enter_mark_select_switch_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-move")) {
    *action = hikari_server_enter_move_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-resize")) {
    *action = hikari_server_enter_resize_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-sheet-assign")) {
    *action = hikari_server_enter_sheet_assign_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-layout")) {
    *action = hikari_server_enter_layout_select_mode;
    *arg = NULL;

  } else if (!strcmp(str, "group-only")) {
    *action = hikari_server_only_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-hide")) {
    *action = hikari_server_hide_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-raise")) {
    *action = hikari_server_raise_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-lower")) {
    *action = hikari_server_lower_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-next")) {
    *action = hikari_server_cycle_next_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-prev")) {
    *action = hikari_server_cycle_prev_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-next")) {
    *action = hikari_server_cycle_next_group_view;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-prev")) {
    *action = hikari_server_cycle_prev_group_view;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-first")) {
    *action = hikari_server_cycle_first_group_view;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-last")) {
    *action = hikari_server_cycle_last_group_view;
    *arg = NULL;

#define PARSE_LAYOUT_BINDING(reg)                                              \
  }                                                                            \
  else if (!strcmp(str, "layout-apply-" #reg))                                 \
  {                                                                            \
    *action = hikari_server_layout_sheet;                                      \
    *arg = (void *)(intptr_t) #reg[0];

    PARSE_LAYOUT_BINDING(a)
    PARSE_LAYOUT_BINDING(b)
    PARSE_LAYOUT_BINDING(c)
    PARSE_LAYOUT_BINDING(d)
    PARSE_LAYOUT_BINDING(e)
    PARSE_LAYOUT_BINDING(f)
    PARSE_LAYOUT_BINDING(g)
    PARSE_LAYOUT_BINDING(h)
    PARSE_LAYOUT_BINDING(i)
    PARSE_LAYOUT_BINDING(j)
    PARSE_LAYOUT_BINDING(k)
    PARSE_LAYOUT_BINDING(l)
    PARSE_LAYOUT_BINDING(m)
    PARSE_LAYOUT_BINDING(n)
    PARSE_LAYOUT_BINDING(o)
    PARSE_LAYOUT_BINDING(p)
    PARSE_LAYOUT_BINDING(q)
    PARSE_LAYOUT_BINDING(r)
    PARSE_LAYOUT_BINDING(s)
    PARSE_LAYOUT_BINDING(t)
    PARSE_LAYOUT_BINDING(u)
    PARSE_LAYOUT_BINDING(v)
    PARSE_LAYOUT_BINDING(w)
    PARSE_LAYOUT_BINDING(x)
    PARSE_LAYOUT_BINDING(y)
    PARSE_LAYOUT_BINDING(z)
    PARSE_LAYOUT_BINDING(0)
    PARSE_LAYOUT_BINDING(1)
    PARSE_LAYOUT_BINDING(2)
    PARSE_LAYOUT_BINDING(3)
    PARSE_LAYOUT_BINDING(4)
    PARSE_LAYOUT_BINDING(5)
    PARSE_LAYOUT_BINDING(6)
    PARSE_LAYOUT_BINDING(7)
    PARSE_LAYOUT_BINDING(8)
    PARSE_LAYOUT_BINDING(9)
#undef PARSE_LAYOUT_BINDING

#define PARSE_MARK_BINDING(mark)                                               \
  }                                                                            \
  else if (!strcmp(str, "mark-show-" #mark))                                   \
  {                                                                            \
    *action = hikari_server_show_mark;                                         \
    *arg = HIKARI_MARK_##mark;                                                 \
  }                                                                            \
  else if (!strcmp(str, "mark-switch-to-" #mark))                              \
  {                                                                            \
    *action = hikari_server_switch_to_mark;                                    \
    *arg = HIKARI_MARK_##mark;

    PARSE_MARK_BINDING(a)
    PARSE_MARK_BINDING(b)
    PARSE_MARK_BINDING(c)
    PARSE_MARK_BINDING(d)
    PARSE_MARK_BINDING(e)
    PARSE_MARK_BINDING(f)
    PARSE_MARK_BINDING(g)
    PARSE_MARK_BINDING(h)
    PARSE_MARK_BINDING(i)
    PARSE_MARK_BINDING(j)
    PARSE_MARK_BINDING(k)
    PARSE_MARK_BINDING(l)
    PARSE_MARK_BINDING(m)
    PARSE_MARK_BINDING(n)
    PARSE_MARK_BINDING(o)
    PARSE_MARK_BINDING(p)
    PARSE_MARK_BINDING(q)
    PARSE_MARK_BINDING(r)
    PARSE_MARK_BINDING(s)
    PARSE_MARK_BINDING(t)
    PARSE_MARK_BINDING(u)
    PARSE_MARK_BINDING(v)
    PARSE_MARK_BINDING(w)
    PARSE_MARK_BINDING(x)
    PARSE_MARK_BINDING(y)
    PARSE_MARK_BINDING(z)
#undef PARSE_MARK_BINDING

#define PARSE_VT_BINDING(n)                                                    \
  }                                                                            \
  else if (!strcmp(str, "vt-switch-to-" #n))                                   \
  {                                                                            \
    *action = hikari_server_session_change_vt;                                 \
    *arg = (void *)(intptr_t)n;

    PARSE_VT_BINDING(1)
    PARSE_VT_BINDING(2)
    PARSE_VT_BINDING(3)
    PARSE_VT_BINDING(4)
    PARSE_VT_BINDING(5)
    PARSE_VT_BINDING(6)
    PARSE_VT_BINDING(7)
    PARSE_VT_BINDING(8)
    PARSE_VT_BINDING(9)
#undef PARSE_VT_BINDING

  } else {
    if (!strncmp(str, "action-", 7)) {
      char *command = NULL;
      const char *action_name = &str[7];
      if ((command = lookup_action(action_configs, action_name)) == NULL) {
        fprintf(stderr,
            "configuration error: unknown action \"%s\"\n",
            action_name);
        goto done;
      } else {
        *action = hikari_server_execute_command;
        *arg = command;
      }
    } else {
      fprintf(stderr, "configuration error: unknown action \"%s\"\n", str);
      goto done;
    }
  }

  success = true;

done:
  return success;
}

void
hikari_action_init(struct hikari_action *action)
{
  action->begin.action = NULL;
  action->begin.arg = NULL;
  action->end.action = NULL;
  action->end.arg = NULL;
}

bool
hikari_action_parse(struct hikari_action *action,
    struct wl_list *action_configs,
    const ucl_object_t *action_obj)
{
  bool success = false;
  ucl_type_t type = ucl_object_type(action_obj);

  if (type == UCL_OBJECT) {
    const ucl_object_t *begin_obj = ucl_object_lookup(action_obj, "begin");
    const ucl_object_t *end_obj = ucl_object_lookup(action_obj, "end");

    struct hikari_event_action *begin = &action->begin;
    struct hikari_event_action *end = &action->end;

    if (begin_obj != NULL) {
      if (!parse_binding(
              action_configs, begin_obj, &begin->action, &begin->arg)) {
        goto done;
      }
    }

    if (end_obj != NULL) {
      if (!parse_binding(action_configs, end_obj, &end->action, &end->arg)) {
        goto done;
      }
    }

  } else if (type == UCL_STRING) {
    struct hikari_event_action *event_action = &action->begin;

    if (!parse_binding(action_configs,
            action_obj,
            &event_action->action,
            &event_action->arg)) {
      goto done;
    }
  }

  success = true;

done:

  return success;
}
