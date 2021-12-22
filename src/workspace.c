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
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/xdg_view.h>
#ifdef HAVE_XWAYLAND
#include <hikari/xwayland_unmanaged_view.h>
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

#ifdef HAVE_LAYERSHELL
  workspace->focus_layer = NULL;
#endif
}

void
hikari_workspace_fini(struct hikari_workspace *workspace)
{
  hikari_free(workspace->sheets);
}

#define CYCLE_WORKSPACE(name)                                                  \
  struct hikari_workspace *hikari_workspace_##name(                            \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    struct hikari_output *output = hikari_output_##name(workspace->output);    \
                                                                               \
    return output->workspace;                                                  \
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

#ifndef NDEBUG
  printf("WORKSPACE MERGE %p INTO %p\n", workspace, into);
#endif

  for (int i = 0; i < HIKARI_NR_OF_SHEETS; i++) {
    struct hikari_sheet *from = &workspace->sheets[i];
    struct hikari_sheet *to = &into->sheets[i];
    struct hikari_view *view, *view_temp;
    wl_list_for_each_reverse_safe (view, view_temp, &from->views, sheet_views) {
      hikari_view_evacuate(view, to);
    }
  }

#ifdef HAVE_XWAYLAND
  struct hikari_xwayland_unmanaged_view *unmanaged_xwayland_view,
      *unmanaged_xwayland_view_temp;
  wl_list_for_each_reverse_safe (unmanaged_xwayland_view,
      unmanaged_xwayland_view_temp,
      &workspace->output->unmanaged_xwayland_views,
      unmanaged_output_views) {
    hikari_xwayland_unmanaged_evacuate(unmanaged_xwayland_view, into);
  }
#endif
}

void
hikari_workspace_quit_view(struct hikari_workspace *workspace)
{
  struct hikari_view *focus_view = workspace->focus_view;

#ifdef HAVE_LAYERSHELL
  struct hikari_layer *focus_layer = workspace->focus_layer;

  if (focus_layer != NULL) {
    assert(focus_view == NULL);
    wlr_layer_surface_v1_destroy(focus_layer->surface);
    return;
  }
#endif

  if (focus_view != NULL) {
#ifdef HAVE_LAYERSHELL
    assert(focus_layer == NULL);
#endif

    hikari_view_quit(focus_view);
    hikari_server_cursor_focus();
  }
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
  hikari_workspace_clear(workspace);

  if (sheet != workspace->sheet) {
    workspace->alternate_sheet = workspace->sheet;
    workspace->sheet = sheet;
  }

  hikari_sheet_show(&workspace->sheets[0]);

  if (sheet->nr != 0) {
    hikari_sheet_show(sheet);
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

#define CYCLE_LAYOUT_VIEW(fallback, link)                                      \
                                                                               \
  struct hikari_view *hikari_workspace_##fallback##_layout_view(               \
      struct hikari_workspace *workspace)                                      \
  {                                                                            \
    struct hikari_layout *layout = workspace->sheet->layout;                   \
                                                                               \
    if (layout == NULL) {                                                      \
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
                                                                               \
    if (layout == NULL) {                                                      \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    struct hikari_view *focus_view = workspace->focus_view;                    \
                                                                               \
    if (focus_view == NULL || !hikari_view_is_tiled(focus_view) ||             \
        focus_view->sheet->layout != layout) {                                 \
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
    return hikari_group_##link##_view(focus_view->group);                      \
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
  assert(hikari_server_in_normal_mode());

  struct wlr_seat *seat = hikari_server.seat;

  struct hikari_workspace *current_workspace = hikari_server.workspace;
  struct hikari_view *focus_view = current_workspace->focus_view;

  if (focus_view != NULL) {
    if (hikari_server_is_indicating()) {
      hikari_group_damage(focus_view->group);
      hikari_indicator_damage(&hikari_server.indicator, focus_view);
    } else {
      hikari_view_damage_border(focus_view);
    }
    hikari_view_activate(focus_view, false);
  }

  wlr_seat_keyboard_end_grab(seat);
  wlr_seat_keyboard_clear_focus(seat);
  wlr_seat_pointer_clear_focus(seat);

#ifdef HAVE_LAYERSHELL
  workspace->focus_layer = NULL;
#endif

  if (view != NULL) {
    assert(!hikari_view_is_hidden(view));

    struct wlr_keyboard *wlr_keyboard = wlr_seat_get_keyboard(seat);

    hikari_view_activate(view, true);

    if (wlr_keyboard != NULL) {
      wlr_seat_keyboard_notify_enter(seat,
          view->surface,
          wlr_keyboard->keycodes,
          wlr_keyboard->num_keycodes,
          &wlr_keyboard->modifiers);
    }

    if (hikari_server_is_indicating()) {
      if (focus_view == NULL || focus_view->group != view->group) {
        hikari_group_damage(view->group);
      }
    } else {
      hikari_view_damage_border(view);
    }

    hikari_indicator_update(&hikari_server.indicator, view);
  } else {
    hikari_cursor_reset_image(&hikari_server.cursor);
  }

  hikari_server.workspace = workspace;
  workspace->focus_view = view;
}

void
hikari_workspace_raise_view(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

  struct hikari_group *group = focus_view->group;
  struct hikari_view *first = hikari_group_first_view(group);

  hikari_view_raise(focus_view);

  hikari_view_damage_border(first);
  hikari_indicator_damage(&hikari_server.indicator, focus_view);
}

void
hikari_workspace_lower_view(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

  hikari_server_unset_cycling();

  hikari_view_lower(focus_view);

  struct hikari_group *group = focus_view->group;
  struct hikari_view *first = hikari_group_first_view(group);

  hikari_view_damage_border(first);

  hikari_server_cursor_focus();
}

void
hikari_workspace_hide_view(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

  hikari_view_hide(focus_view);

  hikari_server_cursor_focus();
}

void
hikari_workspace_only_view(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

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
hikari_workspace_move_resize_view(
    struct hikari_workspace *workspace, int dx, int dy, int dwidth, int dheight)
{
  FOCUS_GUARD(workspace, focus_view);

  hikari_server_set_cycling();

  hikari_view_move_resize(focus_view, dx, dy, dwidth, dheight);
}

#define MOVE(pos)                                                              \
  void hikari_workspace_move_view_##pos(struct hikari_workspace *workspace)    \
  {                                                                            \
    FOCUS_GUARD(workspace, focus_view);                                        \
                                                                               \
    hikari_server_set_cycling();                                               \
                                                                               \
    hikari_view_move_##pos(focus_view);                                        \
  }

MOVE(bottom_left)
MOVE(bottom_middle)
MOVE(bottom_right)
MOVE(center_left)
MOVE(center)
MOVE(center_right)
MOVE(top_left)
MOVE(top_middle)
MOVE(top_right)
#undef MOVE

void
hikari_workspace_snap_view_up(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

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
  FOCUS_GUARD(workspace, focus_view);

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
  FOCUS_GUARD(workspace, focus_view);

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
  FOCUS_GUARD(workspace, focus_view);

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
  FOCUS_GUARD(workspace, focus_view);

  hikari_view_pin_to_sheet(focus_view, sheet);

  if (!hikari_view_is_hidden(focus_view)) {
    struct hikari_output *output = workspace->output;

    hikari_indicator_update_sheet(
        &hikari_server.indicator, output, focus_view->sheet, focus_view->flags);
  }
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
    FOCUS_GUARD(workspace, focus_view);                                        \
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
  FOCUS_GUARD(workspace, focus_view);

  hikari_view_toggle_invisible(focus_view);

  struct hikari_output *output = workspace->output;
  struct hikari_indicator *indicator = &hikari_server.indicator;
  struct wlr_box *geometry = hikari_view_border_geometry(focus_view);

  if (hikari_server_is_indicating()) {
    hikari_indicator_damage_sheet(indicator, output, geometry);
  }

  hikari_indicator_update_sheet(
      indicator, output, focus_view->sheet, focus_view->flags);

  if (hikari_server_is_indicating()) {
    hikari_indicator_damage_sheet(indicator, output, geometry);
  }
}

void
hikari_workspace_toggle_view_public(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

  hikari_view_toggle_public(focus_view);

  struct hikari_output *output = workspace->output;
  struct hikari_indicator *indicator = &hikari_server.indicator;
  struct wlr_box *geometry = hikari_view_border_geometry(focus_view);

  if (hikari_server_is_indicating()) {
    hikari_indicator_damage_sheet(indicator, output, geometry);
  }

  hikari_indicator_update_sheet(
      indicator, output, focus_view->sheet, focus_view->flags);

  if (hikari_server_is_indicating()) {
    hikari_indicator_damage_sheet(indicator, output, geometry);
  }
}

void
hikari_workspace_toggle_view_floating(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

  hikari_view_toggle_floating(focus_view);

  struct hikari_output *output = workspace->output;
  struct hikari_indicator *indicator = &hikari_server.indicator;
  struct wlr_box *geometry = hikari_view_border_geometry(focus_view);

  if (hikari_server_is_indicating()) {
    hikari_indicator_damage_sheet(indicator, output, geometry);
  }

  hikari_indicator_update_sheet(
      indicator, output, focus_view->sheet, focus_view->flags);

  if (hikari_server_is_indicating()) {
    hikari_indicator_damage_sheet(indicator, output, geometry);
  }
}

void
hikari_workspace_show_invisible_sheet_views(struct hikari_workspace *workspace)
{
  hikari_workspace_clear(workspace);
  hikari_sheet_show_invisible(workspace->sheet);
  hikari_server_cursor_focus();
}

void
hikari_workspace_reset_view_geometry(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

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
hikari_workspace_sheet_show_group(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

  struct hikari_group *group = focus_view->group;
  hikari_view_raise(focus_view);
  hikari_workspace_clear(workspace);
  hikari_sheet_show_group(workspace->sheet, group);
  hikari_server_cursor_focus();
}

#define SHOW_VIEWS(cond)                                                       \
  {                                                                            \
    struct hikari_view *view, *view_temp;                                      \
    wl_list_for_each_reverse_safe (                                            \
        view, view_temp, &workspace->output->views, output_views) {            \
      if (cond) {                                                              \
        if (hikari_view_is_hidden(view)) {                                     \
          hikari_view_show(view);                                              \
        } else {                                                               \
          break;                                                               \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

void
hikari_workspace_show_all(struct hikari_workspace *workspace)
{
  hikari_workspace_clear(workspace);
  SHOW_VIEWS(true);
  hikari_server_cursor_focus();
}

void
hikari_workspace_show_invisible(struct hikari_workspace *workspace)
{
  hikari_workspace_clear(workspace);
  SHOW_VIEWS(hikari_view_is_invisible(view));
  hikari_server_cursor_focus();
}

void
hikari_workspace_show_group(struct hikari_workspace *workspace)
{
  FOCUS_GUARD(workspace, focus_view);

  struct hikari_group *group = focus_view->group;
  hikari_workspace_clear(workspace);
  SHOW_VIEWS(view->group == group);
  hikari_server_cursor_focus();
}
#undef SHOW_VIEWS

void
hikari_workspace_show_all_sheet_views(struct hikari_workspace *workspace)
{
  struct hikari_sheet *sheet = workspace->sheet;

  hikari_workspace_clear(workspace);
  hikari_sheet_show_all(sheet);
  hikari_server_cursor_focus();
}

void
hikari_workspace_apply_split(
    struct hikari_workspace *workspace, struct hikari_split *split)
{
  assert(workspace != NULL);
  assert(split != NULL);

  struct hikari_view *focus_view = workspace->focus_view;
  struct hikari_sheet *sheet = workspace->sheet;

  if (focus_view != NULL && focus_view->sheet == sheet &&
      hikari_view_is_tileable(focus_view)) {
    hikari_view_raise(focus_view);

    assert(focus_view == hikari_sheet_first_tileable_view(sheet));
  }

  hikari_workspace_display_sheet_current(workspace);
  hikari_sheet_apply_split(sheet, split);
}

void
hikari_workspace_center_cursor(struct hikari_workspace *workspace)
{
  struct hikari_output *output = workspace->output;
  hikari_cursor_center(&hikari_server.cursor, output, &output->usable_area);
}

struct hikari_view *
hikari_workspace_first_view(struct hikari_workspace *workspace)
{
  assert(workspace != NULL);

  struct hikari_view *view;
  if (workspace->focus_view != NULL) {
    view = workspace->focus_view;
  } else {
    view = hikari_sheet_first_view(workspace->sheet);
  }

  return view;
}
#undef FOCUS_GUARD
