#if !defined(HIKARI_SERVER_H)
#define HIKARI_SERVER_H

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <hikari/group_assign_mode.h>
#include <hikari/indicator.h>
#include <hikari/input_grab_mode.h>
#include <hikari/layout_select_mode.h>
#include <hikari/mark_assign_mode.h>
#include <hikari/mark_select_mode.h>
#include <hikari/move_mode.h>
#include <hikari/normal_mode.h>
#include <hikari/resize_mode.h>
#include <hikari/sheet_assign_mode.h>
#include <hikari/workspace.h>

struct wlr_input_device;
struct wlr_xcursor_manager;

struct hikari_output;
struct hikari_group;

struct hikari_server {
  bool locked;
  bool cycling;
#ifndef NDEBUG
  bool track_damage;
#endif

  const char *socket;
  char *config_path;

  struct hikari_indicator indicator;

  struct wl_display *display;
  struct wl_event_loop *event_loop;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;
  struct wlr_xdg_output_manager_v1 *output_manager;
  struct wlr_data_device_manager *data_device_manager;

  struct wl_listener new_output;
  struct wl_listener new_input;
  struct wl_listener new_xdg_surface;
  struct wl_listener cursor_motion_absolute;
  struct wl_listener cursor_motion;
  struct wl_listener cursor_frame;
  struct wl_listener cursor_axis;
  struct wl_listener cursor_button;
  struct wl_listener request_set_primary_selection;
  struct wl_listener request_set_selection;
  struct wl_listener output_layout_change;
  struct wl_listener new_decoration;
#ifdef HAVE_XWAYLAND
  struct wl_listener new_xwayland_surface;

  struct wlr_xwayland *xwayland;
#endif

  struct wlr_compositor *compositor;
  struct wlr_server_decoration_manager *decoration_manager;

  struct wlr_xdg_shell *xdg_shell;
  struct wlr_output_layout *output_layout;
  struct wlr_seat *seat;

  struct hikari_workspace *workspace;

  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_mgr;

  struct wl_list pointers;
  struct wl_list keyboards;
  struct wl_list outputs;

  struct wl_list groups;
  struct wl_list visible_groups;
  struct wl_list workspaces;

  struct hikari_mode *mode;

  struct hikari_group_assign_mode group_assign_mode;
  struct hikari_input_grab_mode input_grab_mode;
  struct hikari_layout_select_mode layout_select_mode;
  struct hikari_mark_assign_mode mark_assign_mode;
  struct hikari_mark_select_mode mark_select_mode;
  struct hikari_move_mode move_mode;
  struct hikari_normal_mode normal_mode;
  struct hikari_resize_mode resize_mode;
  struct hikari_sheet_assign_mode sheet_assign_mode;

  struct {
    uint32_t modifiers;
    bool mod_released;
    bool mod_changed;
    bool mod_pressed;
  } keyboard_state;
};

extern struct hikari_server hikari_server;

static inline bool
hikari_server_is_cycling(void)
{
  return hikari_server.cycling;
}

static inline void
hikari_server_set_cycling(void)
{
  hikari_server.cycling = true;
}

static inline void
hikari_server_unset_cycling(void)
{
  hikari_server.cycling = false;
}

void
hikari_server_activate_cursor(void);

void
hikari_server_deactivate_cursor(void);

void
hikari_server_start(char *config_path, char *autostart);

void
hikari_server_stop();

void
hikari_server_terminate(void *arg);

void
hikari_server_cursor_focus(void);

void
hikari_server_lock(void *arg);

void
hikari_server_reload(void *arg);

void
hikari_server_execute_command(void *arg);

void
hikari_server_unlock(void);

struct hikari_group *
hikari_server_find_group(const char *group_name);

struct hikari_group *
hikari_server_find_or_create_group(const char *group_name);

#define SHEET_ACTIONS(n)                                                       \
  static inline void hikari_server_display_sheet_##n(void *arg)                \
  {                                                                            \
    hikari_workspace_display_sheet_##n(hikari_server.workspace);               \
  }                                                                            \
                                                                               \
  static inline void hikari_server_pin_view_to_sheet_##n(void *arg)            \
  {                                                                            \
    hikari_workspace_pin_view_to_sheet_##n(hikari_server.workspace);           \
  }

SHEET_ACTIONS(0)
SHEET_ACTIONS(1)
SHEET_ACTIONS(2)
SHEET_ACTIONS(3)
SHEET_ACTIONS(4)
SHEET_ACTIONS(5)
SHEET_ACTIONS(6)
SHEET_ACTIONS(7)
SHEET_ACTIONS(8)
SHEET_ACTIONS(9)
SHEET_ACTIONS(alternate)
SHEET_ACTIONS(current)
SHEET_ACTIONS(next)
SHEET_ACTIONS(prev)
#undef SHEET_ACTIONS

#define MOVE_RESIZE_ACTIONS(dir, x, y)                                         \
  static inline void hikari_server_move_view_##dir(void *arg)                  \
  {                                                                            \
    hikari_workspace_move_view(hikari_server.workspace, x, y);                 \
  }                                                                            \
                                                                               \
  static inline void hikari_server_decrease_view_size_##dir(void *arg)         \
  {                                                                            \
    hikari_workspace_resize_view(hikari_server.workspace, x, y);               \
  }                                                                            \
                                                                               \
  static inline void hikari_server_increase_view_size_##dir(void *arg)         \
  {                                                                            \
    hikari_workspace_resize_view(hikari_server.workspace, x, y);               \
  }                                                                            \
                                                                               \
  static inline void hikari_server_snap_view_##dir(void *arg)                  \
  {                                                                            \
    hikari_workspace_snap_view_##dir(hikari_server.workspace);                 \
  }

MOVE_RESIZE_ACTIONS(up, 0, -100)
MOVE_RESIZE_ACTIONS(down, 0, 100)
MOVE_RESIZE_ACTIONS(left, -100, 0)
MOVE_RESIZE_ACTIONS(right, 100, 0)
#undef MOVE_RESIZE_ACTIONS

#define CYCLE_ACTION(n) void hikari_server_cycle_##n(void *arg);

CYCLE_ACTION(first_group_view)
CYCLE_ACTION(last_group_view)
CYCLE_ACTION(next_group_view)
CYCLE_ACTION(prev_group_view)
CYCLE_ACTION(next_layout_view)
CYCLE_ACTION(prev_layout_view)
CYCLE_ACTION(first_layout_view)
CYCLE_ACTION(last_layout_view)
CYCLE_ACTION(next_group)
CYCLE_ACTION(prev_group)
CYCLE_ACTION(next_view)
CYCLE_ACTION(prev_view)
#undef CYCLE_ACTION

#define WORKSPACE_ACTION(name)                                                 \
  static inline void hikari_server_##name(void *arg)                           \
  {                                                                            \
    hikari_workspace_##name(hikari_server.workspace);                          \
  }

WORKSPACE_ACTION(toggle_view_vertical_maximize)
WORKSPACE_ACTION(toggle_view_horizontal_maximize)
WORKSPACE_ACTION(toggle_view_full_maximize)
WORKSPACE_ACTION(toggle_view_floating)
WORKSPACE_ACTION(toggle_view_invisible)
WORKSPACE_ACTION(only_view)
WORKSPACE_ACTION(only_group)
WORKSPACE_ACTION(hide_view)
WORKSPACE_ACTION(hide_group)
WORKSPACE_ACTION(show_group)
WORKSPACE_ACTION(raise_view)
WORKSPACE_ACTION(raise_group)
WORKSPACE_ACTION(lower_view)
WORKSPACE_ACTION(lower_group)
WORKSPACE_ACTION(show_invisible_sheet_views)
WORKSPACE_ACTION(show_all_invisible_views)
WORKSPACE_ACTION(reset_view_geometry)
WORKSPACE_ACTION(quit_view)
WORKSPACE_ACTION(exchange_next_view)
WORKSPACE_ACTION(exchange_prev_view)
WORKSPACE_ACTION(exchange_main_layout_view)
WORKSPACE_ACTION(show_all_group_views)
WORKSPACE_ACTION(switch_to_next_inhabited_sheet)
WORKSPACE_ACTION(switch_to_prev_inhabited_sheet)
WORKSPACE_ACTION(show_all_sheet_views)
#undef WORKSPACE_ACTION

static inline void
hikari_server_clear_workspace(void *arg)
{
  hikari_workspace_clear(hikari_server.workspace);
}

#define MODE(name)                                                             \
  void hikari_server_enter_##name##_mode(void *arg);                           \
                                                                               \
  static inline bool hikari_server_in_##name##_mode(void)                      \
  {                                                                            \
    return hikari_server.mode ==                                               \
           (struct hikari_mode *)&hikari_server.name##_mode;                   \
  }

MODE(group_assign)
MODE(input_grab)
MODE(layout_select)
MODE(mark_assign)
MODE(mark_select)
MODE(move)
MODE(normal)
MODE(resize)
MODE(sheet_assign)

void
hikari_server_enter_mark_select_switch_mode(void *arg);
#undef MODE

void
hikari_server_refresh_indication(void);

void
hikari_server_reset_sheet_layout(void *arg);

void
hikari_server_layout_sheet(void *arg);

void
hikari_server_session_change_vt(void *arg);

void
hikari_server_show_mark(void *arg);

void
hikari_server_switch_to_mark(void *arg);

#endif
