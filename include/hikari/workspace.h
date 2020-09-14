#if !defined(HIKARI_WORKSPACE_H)
#define HIKARI_WORKSPACE_H

#include <wayland-server-core.h>
#include <wayland-util.h>

#ifdef HAVE_LAYERSHELL
struct hikari_layer;
#endif
struct hikari_output;
struct hikari_renderer;
struct hikari_sheet;
struct hikari_split;
struct hikari_view;

struct wlr_texture;

struct hikari_workspace {
  struct hikari_sheet *sheet;
  struct hikari_sheet *alternate_sheet;
  struct hikari_output *output;
  struct hikari_sheet *sheets;
  struct hikari_view *focus_view;

#ifdef HAVE_LAYERSHELL
  struct hikari_layer *focus_layer;
#endif

  struct wl_list views;
};

void
hikari_workspace_init(
    struct hikari_workspace *workspace, struct hikari_output *output);

void
hikari_workspace_fini(struct hikari_workspace *workspace);

struct hikari_workspace *
hikari_workspace_next(struct hikari_workspace *workspace);

struct hikari_workspace *
hikari_workspace_prev(struct hikari_workspace *workspace);

void
hikari_workspace_merge(
    struct hikari_workspace *workspace, struct hikari_workspace *into);

void
hikari_workspace_focus_view(
    struct hikari_workspace *workspace, struct hikari_view *view);

void
hikari_workspace_move_view(struct hikari_workspace *workspace, int dx, int dy);

void
hikari_workspace_move_view_bottom_left(struct hikari_workspace *workspace);

void
hikari_workspace_move_view_bottom_middle(struct hikari_workspace *workspace);

void
hikari_workspace_move_view_bottom_right(struct hikari_workspace *workspace);

void
hikari_workspace_move_view_center_left(struct hikari_workspace *workspace);

void
hikari_workspace_move_view_center(struct hikari_workspace *workspace);

void
hikari_workspace_move_view_center_right(struct hikari_workspace *workspace);

void
hikari_workspace_move_view_top_left(struct hikari_workspace *workspace);

void
hikari_workspace_move_view_top_middle(struct hikari_workspace *workspace);

void
hikari_workspace_move_view_top_right(struct hikari_workspace *workspace);

void
hikari_workspace_move_resize_view(struct hikari_workspace *workspace,
    int dx,
    int dy,
    int dwidth,
    int dheight);

void
hikari_workspace_center_cursor(struct hikari_workspace *workspace);

struct hikari_view *
hikari_workspace_first_view(struct hikari_workspace *workspace);

#define WORKSPACE_CYCLE_ACTION(name)                                           \
  struct hikari_view *hikari_workspace_##name(                                 \
      struct hikari_workspace *workspace);

WORKSPACE_CYCLE_ACTION(next_view)
WORKSPACE_CYCLE_ACTION(prev_view)
WORKSPACE_CYCLE_ACTION(next_layout_view)
WORKSPACE_CYCLE_ACTION(prev_layout_view)
WORKSPACE_CYCLE_ACTION(first_layout_view)
WORKSPACE_CYCLE_ACTION(last_layout_view)
WORKSPACE_CYCLE_ACTION(first_group_view)
WORKSPACE_CYCLE_ACTION(last_group_view)
WORKSPACE_CYCLE_ACTION(next_group_view)
WORKSPACE_CYCLE_ACTION(prev_group_view)
WORKSPACE_CYCLE_ACTION(next_group)
WORKSPACE_CYCLE_ACTION(prev_group)
WORKSPACE_CYCLE_ACTION(next_sheet_view)
WORKSPACE_CYCLE_ACTION(prev_sheet_view)
#undef WORKSPACE_CYCLE_ACTION

#define WORKSPACE_ACTION(name)                                                 \
  void hikari_workspace_##name(struct hikari_workspace *workspace);

WORKSPACE_ACTION(clear)
WORKSPACE_ACTION(quit_view)
WORKSPACE_ACTION(display_sheet_0)
WORKSPACE_ACTION(display_sheet_1)
WORKSPACE_ACTION(display_sheet_2)
WORKSPACE_ACTION(display_sheet_3)
WORKSPACE_ACTION(display_sheet_4)
WORKSPACE_ACTION(display_sheet_5)
WORKSPACE_ACTION(display_sheet_6)
WORKSPACE_ACTION(display_sheet_7)
WORKSPACE_ACTION(display_sheet_8)
WORKSPACE_ACTION(display_sheet_9)
WORKSPACE_ACTION(display_sheet_alternate)
WORKSPACE_ACTION(display_sheet_current)
WORKSPACE_ACTION(display_sheet_next)
WORKSPACE_ACTION(display_sheet_prev)
WORKSPACE_ACTION(switch_to_next_sheet)
WORKSPACE_ACTION(switch_to_prev_sheet)
WORKSPACE_ACTION(switch_to_next_inhabited_sheet)
WORKSPACE_ACTION(switch_to_prev_inhabited_sheet)
WORKSPACE_ACTION(raise_view)
WORKSPACE_ACTION(lower_view)
WORKSPACE_ACTION(hide_view)
WORKSPACE_ACTION(snap_view_up)
WORKSPACE_ACTION(snap_view_down)
WORKSPACE_ACTION(snap_view_left)
WORKSPACE_ACTION(snap_view_right)
WORKSPACE_ACTION(only_view)
WORKSPACE_ACTION(pin_view_to_sheet_0)
WORKSPACE_ACTION(pin_view_to_sheet_1)
WORKSPACE_ACTION(pin_view_to_sheet_2)
WORKSPACE_ACTION(pin_view_to_sheet_3)
WORKSPACE_ACTION(pin_view_to_sheet_4)
WORKSPACE_ACTION(pin_view_to_sheet_5)
WORKSPACE_ACTION(pin_view_to_sheet_6)
WORKSPACE_ACTION(pin_view_to_sheet_7)
WORKSPACE_ACTION(pin_view_to_sheet_8)
WORKSPACE_ACTION(pin_view_to_sheet_9)
WORKSPACE_ACTION(pin_view_to_sheet_alternate)
WORKSPACE_ACTION(pin_view_to_sheet_current)
WORKSPACE_ACTION(pin_view_to_sheet_next)
WORKSPACE_ACTION(pin_view_to_sheet_prev)
WORKSPACE_ACTION(toggle_view_invisible)
WORKSPACE_ACTION(toggle_view_floating)
WORKSPACE_ACTION(toggle_view_public)
WORKSPACE_ACTION(toggle_view_full_maximize)
WORKSPACE_ACTION(toggle_view_vertical_maximize)
WORKSPACE_ACTION(toggle_view_horizontal_maximize)
WORKSPACE_ACTION(toggle_view_horizontal_maximize)
WORKSPACE_ACTION(reset_view_geometry)
WORKSPACE_ACTION(show_invisible_sheet_views)
WORKSPACE_ACTION(exchange_next_view)
WORKSPACE_ACTION(exchange_prev_view)
WORKSPACE_ACTION(exchange_main_layout_view)
WORKSPACE_ACTION(sheet_show_group)
WORKSPACE_ACTION(show_all_sheet_views)
WORKSPACE_ACTION(show_all)
WORKSPACE_ACTION(show_group)
WORKSPACE_ACTION(show_invisible)
#undef WORKSPACE_ACTION

void
hikari_workspace_switch_sheet(
    struct hikari_workspace *workspace, struct hikari_sheet *sheet);

void
hikari_workspace_apply_split(
    struct hikari_workspace *workspace, struct hikari_split *split);

#endif
