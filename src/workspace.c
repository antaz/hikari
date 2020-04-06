#include <hikari/workspace.h>

#include <stdbool.h>
#include <stdlib.h>

#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_matrix.h>
#ifdef HAVE_XWAYLAND
#include <wlr/xwayland.h>
#endif

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/group_assign_mode.h>
#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/input_grab_mode.h>
#include <hikari/layout.h>
#include <hikari/mark_assign_mode.h>
#include <hikari/mark_select_mode.h>
#include <hikari/normal_mode.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/xdg_view.h>
#ifdef HAVE_XWAYLAND
#include <hikari/xwayland_view.h>
#endif

#define FOCUS_GUARD(workspace, view)                                           \
  struct hikari_view *view = workspace->focus_view;                            \
                                                                               \
  if (focus_view == NULL) {                                                    \
    return;                                                                    \
  }

void
hikari_workspace_init(
    struct hikari_workspace *workspace, struct hikari_output *output)
{
  wl_list_init(&workspace->views);
  wl_list_init(&hikari_server.visible_groups);
  workspace->output = output;
  workspace->focus_view = NULL;
  workspace->sheets =
      hikari_calloc(HIKARI_NR_OF_SHEETS, sizeof(struct hikari_sheet));

  for (int i = 0; i < HIKARI_NR_OF_SHEETS; i++) {
    hikari_sheet_init(&workspace->sheets[i], i, workspace);
  }

  workspace->alternate_sheet = &workspace->sheets[0];
  workspace->sheet = &workspace->sheets[1];

  wl_list_insert(hikari_server.workspaces.prev, &workspace->server_workspaces);
}

void
hikari_workspace_fini(struct hikari_workspace *workspace)
{
  hikari_free(workspace->sheets);

  wl_list_remove(&workspace->server_workspaces);
}

#define CYCLE_WORKSPACE(name)                                                  \
  struct hikari_workspace *hikari_workspace_##name(                            \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    struct wl_list *name = workspace->server_workspaces.name;                  \
                                                                               \
    if (name == &hikari_server.workspaces) {                                   \
      name = hikari_server.workspaces.name;                                    \
    }                                                                          \
                                                                               \
    struct hikari_workspace *name##_workspace =                                \
        wl_container_of(name, name##_workspace, server_workspaces);            \
                                                                               \
    return name##_workspace;                                                   \
  }

CYCLE_WORKSPACE(next)
CYCLE_WORKSPACE(prev)
#undef CYCLE_WORKSPACE

void
hikari_workspace_merge(
    struct hikari_workspace *workspace, struct hikari_workspace *into)
{
  assert(workspace != NULL);
  assert(into != NULL);

  for (int i = 0; i < HIKARI_NR_OF_SHEETS; i++) {
    struct hikari_sheet *from = &workspace->sheets[i];
    struct hikari_sheet *to = &into->sheets[i];
    struct hikari_view *view, *view_temp;
    wl_list_for_each_reverse_safe (view, view_temp, &from->views, sheet_views) {
      hikari_view_migrate_to_sheet(view, to);
    }
  }
}

void
hikari_workspace_quit_view(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_view_quit(focus_view);
  hikari_server_cursor_focus();
}

void
hikari_workspace_clear(struct hikari_workspace *workspace)
{
  struct hikari_view *view = NULL, *view_tmp = NULL;

  wl_list_for_each_reverse_safe (
      view, view_tmp, &(workspace->views), workspace_views) {
    hikari_view_hide(view);
  }

  hikari_server_cursor_focus();
}

static void
display_sheet(struct hikari_workspace *workspace, struct hikari_sheet *sheet)
{
  struct hikari_view *view = NULL;

  hikari_workspace_clear(workspace);

  if (sheet != workspace->sheet) {
    workspace->alternate_sheet = workspace->sheet;
    workspace->sheet = sheet;
  }

  struct hikari_sheet *sheets = workspace->sheets;

  wl_list_for_each_reverse (view, &sheets[0].views, sheet_views) {
    if (!hikari_view_is_invisible(view)) {
      hikari_view_show(view);
    }
  }

  if (sheet->nr != 0) {
    view = NULL;
    wl_list_for_each_reverse (view, &sheet->views, sheet_views) {
      if (!hikari_view_is_invisible(view)) {
        hikari_view_show(view);
      }
    }
  }

  hikari_server_cursor_focus();
}

#define DISPLAY_SHEET(name, sheet)                                             \
  void hikari_workspace_display_sheet_##name(                                  \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    display_sheet(workspace, sheet);                                           \
  }

DISPLAY_SHEET(0, &workspace->sheets[0])
DISPLAY_SHEET(1, &workspace->sheets[1])
DISPLAY_SHEET(2, &workspace->sheets[2])
DISPLAY_SHEET(3, &workspace->sheets[3])
DISPLAY_SHEET(4, &workspace->sheets[4])
DISPLAY_SHEET(5, &workspace->sheets[5])
DISPLAY_SHEET(6, &workspace->sheets[6])
DISPLAY_SHEET(7, &workspace->sheets[7])
DISPLAY_SHEET(8, &workspace->sheets[8])
DISPLAY_SHEET(9, &workspace->sheets[9])
DISPLAY_SHEET(alternate, workspace->alternate_sheet)
DISPLAY_SHEET(current, workspace->sheet)
DISPLAY_SHEET(next, hikari_sheet_next(workspace->sheet))
DISPLAY_SHEET(prev, hikari_sheet_prev(workspace->sheet))
#undef DISPLAY_SHEET

void
hikari_workspace_switch_sheet(
    struct hikari_workspace *workspace, struct hikari_sheet *sheet)
{
  assert(workspace == sheet->workspace);

  display_sheet(workspace, sheet);
}

void
hikari_workspace_switch_to_next_sheet(struct hikari_workspace *workspace)
{
  if (workspace->sheet->nr == 0) {
    return;
  }

  uint8_t nr = workspace->sheet->nr == 9 ? 1 : workspace->sheet->nr + 1;

  display_sheet(workspace, &workspace->sheets[nr]);
}

void
hikari_workspace_switch_to_prev_sheet(struct hikari_workspace *workspace)
{
  if (workspace->sheet->nr == 0) {
    return;
  }

  uint8_t nr = workspace->sheet->nr == 1 ? 9 : workspace->sheet->nr - 1;

  display_sheet(workspace, &workspace->sheets[nr]);
}

void
hikari_workspace_switch_to_next_inhabited_sheet(
    struct hikari_workspace *workspace)
{
  struct hikari_sheet *sheet = hikari_sheet_next_inhabited(workspace->sheet);

  if (sheet == workspace->sheet) {
    return;
  }

  display_sheet(workspace, sheet);
}

void
hikari_workspace_switch_to_prev_inhabited_sheet(
    struct hikari_workspace *workspace)
{
  struct hikari_sheet *sheet = hikari_sheet_prev_inhabited(workspace->sheet);

  if (sheet == workspace->sheet) {
    return;
  }

  display_sheet(workspace, sheet);
}

struct hikari_view *
hikari_workspace_first_view(struct hikari_workspace *workspace)
{
  struct hikari_view *view;

  if (wl_list_empty(&workspace->views)) {
    return NULL;
  }

  view = wl_container_of(workspace->views.next, view, workspace_views);

  return view;
}

struct hikari_view *
hikari_workspace_last_view(struct hikari_workspace *workspace)
{
  struct hikari_view *view;

  if (wl_list_empty(&workspace->views)) {
    return NULL;
  }

  view = wl_container_of(workspace->views.prev, view, workspace_views);

  return view;
}

#define CYCLE_VIEW(name, link)                                                 \
  struct hikari_view *hikari_workspace_##name##_view(                          \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    if (wl_list_empty(&workspace->views)) {                                    \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    struct hikari_view *view;                                                  \
    struct hikari_view *focus_view = workspace->focus_view;                    \
                                                                               \
    if (focus_view == NULL) {                                                  \
      view = wl_container_of(workspace->views.link, view, workspace_views);    \
    } else {                                                                   \
      struct wl_list *link = focus_view->workspace_views.link;                 \
                                                                               \
      if (link != &workspace->views) {                                         \
        view = wl_container_of(link, view, workspace_views);                   \
      } else {                                                                 \
        view = wl_container_of(workspace->views.link, view, workspace_views);  \
      }                                                                        \
    }                                                                          \
                                                                               \
    return view;                                                               \
  }

CYCLE_VIEW(next, prev)
CYCLE_VIEW(prev, next)
#undef CYCLE_VIEW

#define CYCLE_LAYOUT_VIEW(fallback, link)                                      \
                                                                               \
  struct hikari_view *hikari_workspace_##fallback##_layout_view(               \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    struct hikari_layout *layout = workspace->sheet->layout;                   \
    if (workspace->sheet->layout == NULL) {                                    \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    return hikari_layout_##fallback##_view(layout);                            \
  }                                                                            \
                                                                               \
  struct hikari_view *hikari_workspace_##link##_layout_view(                   \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    struct hikari_layout *layout = workspace->sheet->layout;                   \
    if (workspace->sheet->layout == NULL) {                                    \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    struct hikari_view *focus_view = workspace->focus_view;                    \
                                                                               \
    if (focus_view == NULL || !hikari_view_is_tiled(focus_view)) {             \
      return hikari_layout_##fallback##_view(layout);                          \
    } else {                                                                   \
      return hikari_tile_##link##_view(focus_view->tile);                      \
    }                                                                          \
  }

CYCLE_LAYOUT_VIEW(first, next)
CYCLE_LAYOUT_VIEW(last, prev)
#undef CYCLE_LAYOUT_VIEW

#define CYCLE_GROUP_VIEW(link)                                                 \
  struct hikari_view *hikari_workspace_##link##_group_view(                    \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    struct hikari_view *focus_view = workspace->focus_view;                    \
    if (focus_view == NULL) {                                                  \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    return hikari_group_##link##_view(                                         \
        focus_view->group, hikari_server.workspace);                           \
  }

CYCLE_GROUP_VIEW(first)
CYCLE_GROUP_VIEW(last)
#undef CYCLE_GROUP_VIEW

#define CYCLE_GROUP_VIEW(name, link)                                           \
  struct hikari_view *hikari_workspace_##name##_group_view(                    \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    struct hikari_view *focus_view = workspace->focus_view;                    \
    if (focus_view == NULL) {                                                  \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    struct hikari_view *view;                                                  \
    struct hikari_group *group = focus_view->group;                            \
    struct wl_list *link = focus_view->visible_group_views.link;               \
                                                                               \
    if (link != &group->visible_views) {                                       \
      view = wl_container_of(                                                  \
          focus_view->visible_group_views.link, view, visible_group_views);    \
    } else {                                                                   \
      view = wl_container_of(                                                  \
          group->visible_views.link, view, visible_group_views);               \
    }                                                                          \
                                                                               \
    return view;                                                               \
  }

CYCLE_GROUP_VIEW(next, prev)
CYCLE_GROUP_VIEW(prev, next)
#undef CYCLE_GROUP_VIEW

#define CYCLE_GROUP(name, link)                                                \
  struct hikari_view *hikari_workspace_##name##_group(                         \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    if (wl_list_empty(&hikari_server.visible_groups)) {                        \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    struct hikari_view *focus_view = workspace->focus_view;                    \
                                                                               \
    struct hikari_group *group;                                                \
    if (focus_view == NULL) {                                                  \
      group = wl_container_of(                                                 \
          hikari_server.visible_groups.link, group, visible_server_groups);    \
    } else {                                                                   \
      struct wl_list *link = focus_view->group->visible_server_groups.link;    \
                                                                               \
      if (link != &hikari_server.visible_groups) {                             \
        group = wl_container_of(link, group, visible_server_groups);           \
      } else {                                                                 \
        group = wl_container_of(                                               \
            hikari_server.visible_groups.link, group, visible_server_groups);  \
      }                                                                        \
                                                                               \
      if (group == focus_view->group) {                                        \
        return focus_view;                                                     \
      }                                                                        \
    }                                                                          \
                                                                               \
    struct hikari_view *view =                                                 \
        wl_container_of(group->visible_views.next, view, visible_group_views); \
                                                                               \
    return view;                                                               \
  }

CYCLE_GROUP(next, prev)
CYCLE_GROUP(prev, next)
#undef CYCLE_GROUP

void
hikari_workspace_focus_view(
    struct hikari_workspace *workspace, struct hikari_view *view)
{
  assert(hikari_server.workspace != NULL);

  struct wlr_seat *seat = hikari_server.seat;

  if (!hikari_server_in_normal_mode()) {
    hikari_server_enter_normal_mode(NULL);
  }

  struct hikari_workspace *current_workspace = hikari_server.workspace;
  struct hikari_view *focus_view = current_workspace->focus_view;

  if (focus_view != NULL) {
    hikari_indicator_damage(&hikari_server.indicator, focus_view);

    hikari_view_activate(focus_view, false);
  }

  wlr_seat_keyboard_end_grab(seat);

  if (view != NULL) {
    assert(!hikari_view_is_hidden(view));

    hikari_view_activate(view, true);
    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);

    wlr_seat_keyboard_notify_enter(seat,
        view->surface,
        keyboard->keycodes,
        keyboard->num_keycodes,
        &keyboard->modifiers);

    hikari_indicator_update(&hikari_server.indicator,
        view,
        hikari_configuration->indicator_selected);
  } else {
    wlr_seat_keyboard_clear_focus(seat);
  }

  hikari_server.workspace = workspace;
  workspace->focus_view = view;

  hikari_server_refresh_indication();
}

void
hikari_workspace_raise_view(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_view_raise(focus_view);

  hikari_server_refresh_indication();
}

void
hikari_workspace_raise_group(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

  hikari_group_raise(focus_view->group, focus_view);

  hikari_server_refresh_indication();
}

void
hikari_workspace_lower_view(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_view_lower(focus_view);

  hikari_server_cursor_focus();
  hikari_server_refresh_indication();
}

void
hikari_workspace_lower_group(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

  hikari_group_lower(focus_view->group, focus_view);

  hikari_server_cursor_focus();
}

void
hikari_workspace_hide_view(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_view_hide(focus_view);

  hikari_server_cursor_focus();
}

void
hikari_workspace_move_view(struct hikari_workspace *workspace, int dx, int dy)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_server_set_cycling();

  // TODO this is the only place we need this
  hikari_view_move(focus_view, dx, dy);
}

void
hikari_workspace_only_view(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  struct hikari_view *view = NULL;
  struct hikari_view *view_tmp;
  wl_list_for_each_safe (view, view_tmp, &workspace->views, workspace_views) {
    if (view != focus_view) {
      hikari_view_hide(view);
    }
  }

  hikari_server_cursor_focus();
}

void
hikari_workspace_resize_view(
    struct hikari_workspace *workspace, int dwidth, int dheight)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_server_set_cycling();

  hikari_view_resize(focus_view, dwidth, dheight);
}

void
hikari_workspace_snap_view_up(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_server_set_cycling();

  struct wlr_box *view_geometry = hikari_view_geometry(focus_view);

  int border = hikari_configuration->border;
  int gap = hikari_configuration->gap;
  int lookahead = view_geometry->y - border - gap;
  int view_right = view_geometry->x + view_geometry->width;
  int y;

  bool found = false;
  struct hikari_view *view = NULL;
  struct wlr_box *geometry = NULL;
  wl_list_for_each (view, &workspace->views, workspace_views) {
    if (view == focus_view) {
      continue;
    }

    geometry = hikari_view_geometry(view);

    int right = geometry->x + geometry->width;

    if (geometry->y + geometry->height + border < lookahead &&
        ((view_right >= geometry->x && geometry->x >= view_geometry->x) ||
            (right >= view_geometry->x && view_geometry->x >= geometry->x))) {
      found = true;
      y = geometry->y + gap + geometry->height + border;
      break;
    }
  }

  if (!found) {
    struct hikari_output *output = focus_view->output;
    y = border + output->usable_area.y;
  }

  hikari_view_move_absolute(focus_view, view_geometry->x, y);
}

void
hikari_workspace_snap_view_down(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_server_set_cycling();

  struct wlr_box *view_geometry = hikari_view_geometry(focus_view);

  int border = hikari_configuration->border;
  int gap = hikari_configuration->gap;
  int lookahead = view_geometry->y + view_geometry->height + border + gap;
  int view_right = view_geometry->x + view_geometry->width;
  int y;

  bool found = false;
  struct hikari_view *view = NULL;
  struct wlr_box *geometry = NULL;
  wl_list_for_each (view, &workspace->views, workspace_views) {
    if (view == focus_view) {
      continue;
    }

    geometry = hikari_view_geometry(view);

    int right = geometry->x + geometry->width;

    if (geometry->y > lookahead &&
        ((view_right >= geometry->x && geometry->x >= view_geometry->x) ||
            (right >= view_geometry->x && view_geometry->x >= geometry->x))) {
      found = true;
      y = geometry->y - gap - border - view_geometry->height;
      break;
    }
  }

  if (!found) {
    struct hikari_output *output = focus_view->output;
    y = output->usable_area.y + output->usable_area.height -
        view_geometry->height - border;
  }

  hikari_view_move_absolute(focus_view, view_geometry->x, y);
}

void
hikari_workspace_snap_view_left(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_server_set_cycling();

  struct wlr_box *view_geometry = hikari_view_geometry(focus_view);

  int border = hikari_configuration->border;
  int gap = hikari_configuration->gap;
  int lookahead = view_geometry->x - gap - border;
  int view_bottom = view_geometry->y + view_geometry->height;
  int x;

  bool found = false;
  struct hikari_view *view = NULL;
  struct wlr_box *geometry = NULL;
  wl_list_for_each (view, &workspace->views, workspace_views) {
    if (view == focus_view) {
      continue;
    }

    geometry = hikari_view_geometry(view);

    int bottom = geometry->y + geometry->height;

    if (geometry->x + geometry->width + border < lookahead &&
        ((view_bottom >= geometry->y && geometry->y >= view_geometry->y) ||
            (bottom >= view_geometry->y && view_geometry->y >= geometry->y))) {
      found = true;
      x = geometry->x + geometry->width + gap + border;
      break;
    }
  }

  if (!found) {
    struct hikari_output *output = focus_view->output;
    x = border + output->usable_area.x;
  }

  hikari_view_move_absolute(focus_view, x, view_geometry->y);
}

void
hikari_workspace_snap_view_right(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_server_set_cycling();

  struct wlr_box *view_geometry = hikari_view_geometry(focus_view);

  int border = hikari_configuration->border;
  int gap = hikari_configuration->gap;
  int lookahead = view_geometry->x + view_geometry->width + gap + border;
  int view_bottom = view_geometry->y + view_geometry->height;
  int x;

  bool found = false;
  struct hikari_view *view = NULL;
  struct wlr_box *geometry = NULL;
  wl_list_for_each (view, &workspace->views, workspace_views) {
    if (view == focus_view) {
      continue;
    }

    geometry = hikari_view_geometry(view);

    int bottom = geometry->y + geometry->height;

    if (geometry->x > lookahead &&
        ((view_bottom >= geometry->y && geometry->y >= view_geometry->y) ||
            (bottom >= view_geometry->y && view_geometry->y >= geometry->y))) {
      found = true;
      x = geometry->x - gap - view_geometry->width - border;
      break;
    }
  }

  if (!found) {
    struct hikari_output *output = focus_view->output;
    x = output->usable_area.x + output->usable_area.width -
        view_geometry->width - border;
  }

  hikari_view_move_absolute(focus_view, x, view_geometry->y);
}

static void
pin_to_sheet(struct hikari_workspace *workspace, struct hikari_sheet *sheet)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_view_pin_to_sheet(focus_view, sheet);

  if (!hikari_view_is_hidden(focus_view)) {
    struct wlr_box *geometry = hikari_view_geometry(focus_view);
    struct hikari_output *output = workspace->output;

    hikari_indicator_update_sheet(&hikari_server.indicator,
        geometry,
        output,
        focus_view->sheet,
        hikari_configuration->indicator_selected,
        hikari_view_is_invisible(focus_view),
        hikari_view_is_floating(focus_view));
  }

  hikari_server_refresh_indication();
}

#define PIN_TO_SHEET(name, sheet)                                              \
  void hikari_workspace_pin_view_to_sheet_##name(                              \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    pin_to_sheet(workspace, sheet);                                            \
  }

PIN_TO_SHEET(0, &workspace->sheets[0])
PIN_TO_SHEET(1, &workspace->sheets[1])
PIN_TO_SHEET(2, &workspace->sheets[2])
PIN_TO_SHEET(3, &workspace->sheets[3])
PIN_TO_SHEET(4, &workspace->sheets[4])
PIN_TO_SHEET(5, &workspace->sheets[5])
PIN_TO_SHEET(6, &workspace->sheets[6])
PIN_TO_SHEET(7, &workspace->sheets[7])
PIN_TO_SHEET(8, &workspace->sheets[8])
PIN_TO_SHEET(9, &workspace->sheets[9])
PIN_TO_SHEET(alternate, workspace->alternate_sheet)
PIN_TO_SHEET(current, workspace->sheet)
PIN_TO_SHEET(next, hikari_sheet_next(workspace->sheet))
PIN_TO_SHEET(prev, hikari_sheet_prev(workspace->sheet))
#undef PIN_TO_SHEET

#define MAXIMIZE(n)                                                            \
  void hikari_workspace_toggle_view_##n##_maximize(                            \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    FOCUS_GUARD(workspace, focus_view)                                         \
                                                                               \
    hikari_view_toggle_##n##_maximize(focus_view);                             \
  }

MAXIMIZE(full)
MAXIMIZE(vertical)
MAXIMIZE(horizontal)
#undef MAXIMIZE

void
hikari_workspace_toggle_view_invisible(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_view_toggle_invisible(focus_view);

  struct wlr_box *geometry = hikari_view_geometry(focus_view);
  struct hikari_output *output = workspace->output;

  hikari_indicator_update_sheet(&hikari_server.indicator,
      geometry,
      output,
      focus_view->sheet,
      hikari_configuration->indicator_selected,
      hikari_view_is_invisible(focus_view),
      hikari_view_is_floating(focus_view));
}

void
hikari_workspace_toggle_view_floating(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_view_toggle_floating(focus_view);

  struct wlr_box *geometry = hikari_view_geometry(focus_view);
  struct hikari_output *output = workspace->output;

  hikari_indicator_update_sheet(&hikari_server.indicator,
      geometry,
      output,
      focus_view->sheet,
      hikari_configuration->indicator_selected,
      hikari_view_is_invisible(focus_view),
      hikari_view_is_floating(focus_view));
}

void
hikari_workspace_show_invisible_sheet_views(struct hikari_workspace *workspace)
{
  hikari_workspace_clear(workspace);

  struct hikari_view *view = NULL;
  wl_list_for_each_reverse (view, &workspace->sheet->views, sheet_views) {
    if (hikari_view_is_invisible(view)) {
      hikari_view_show(view);
    }
  }

  hikari_server_cursor_focus();
}

void
hikari_workspace_show_all_invisible_views(struct hikari_workspace *workspace)
{
  hikari_workspace_clear(workspace);

  struct hikari_view *view = NULL;
  wl_list_for_each_reverse (view, &workspace->output->views, output_views) {
    if (hikari_view_is_invisible(view)) {
      hikari_view_show(view);
    }
  }

  hikari_server_cursor_focus();
}

void
hikari_workspace_reset_view_geometry(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  hikari_view_reset_geometry(focus_view);
}

#define VIEW_EXCHANGE(link)                                                    \
  void hikari_workspace_exchange_##link##_view(                                \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    struct hikari_view *focus_view = workspace->focus_view;                    \
    struct hikari_layout *layout = workspace->sheet->layout;                   \
                                                                               \
    if (focus_view == NULL || layout == NULL ||                                \
        !hikari_view_is_tiled(focus_view)) {                                   \
      return;                                                                  \
    }                                                                          \
                                                                               \
    struct hikari_view *link = hikari_tile_##link##_view(focus_view->tile);    \
                                                                               \
    assert(!hikari_view_is_hidden(link));                                      \
                                                                               \
    if (focus_view == link) {                                                  \
      return;                                                                  \
    }                                                                          \
                                                                               \
    hikari_view_exchange(focus_view, link);                                    \
    hikari_view_center_cursor(link);                                           \
  }

VIEW_EXCHANGE(prev)
VIEW_EXCHANGE(next)
#undef VIEW_EXCHANGE

void
hikari_workspace_exchange_main_layout_view(struct hikari_workspace *workspace)
{
  struct hikari_view *focus_view = workspace->focus_view;
  struct hikari_layout *layout = workspace->sheet->layout;

  if (focus_view == NULL || layout == NULL ||
      !hikari_view_is_tiled(focus_view) || hikari_view_is_dirty(focus_view)) {
    return;
  }

  struct hikari_view *first = hikari_layout_first_view(layout);

  if (focus_view == first || hikari_view_is_hidden(first) ||
      hikari_view_is_dirty(first)) {
    return;
  }

  hikari_view_exchange(focus_view, first);
  hikari_view_center_cursor(first);
}

void
hikari_workspace_only_group(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  struct hikari_group *group = focus_view->group;

  struct hikari_view *view = NULL, *view_temp = NULL;
  wl_list_for_each_safe (view, view_temp, &workspace->views, workspace_views) {
    if (view->group != group) {
      hikari_view_hide(view);
    }
  }
}

void
hikari_workspace_show_group(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  struct hikari_group *group = focus_view->group;

  hikari_workspace_clear(workspace);

  struct hikari_view *view = NULL;
  wl_list_for_each_reverse (view, &workspace->sheet->views, sheet_views) {
    if (view->group == group) {
      hikari_view_show(view);
    }
  }

  hikari_server_cursor_focus();
}

void
hikari_workspace_hide_group(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  struct hikari_group *group = focus_view->group;

  struct hikari_view *view = NULL, *view_temp = NULL;
  wl_list_for_each_safe (
      view, view_temp, &group->visible_views, visible_group_views) {
    hikari_view_hide(view);
  }

  hikari_server_cursor_focus();
}

void
hikari_workspace_show_all_group_views(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view)

  struct hikari_group *group = focus_view->group;

  hikari_workspace_clear(workspace);

  struct hikari_view *view = NULL;
  wl_list_for_each_reverse (view, &group->views, group_views) {
    hikari_view_show(view);
  }

  hikari_server_cursor_focus();
}

void
hikari_workspace_show_all_sheet_views(struct hikari_workspace *workspace)
{
  struct hikari_view *view;
  struct hikari_sheet *sheet = workspace->sheet;

  hikari_workspace_clear(workspace);

  view = NULL;
  wl_list_for_each_reverse (view, &sheet->views, sheet_views) {
    hikari_view_show(view);
  }

  hikari_server_cursor_focus();
}

#undef FOCUS_GUARD
