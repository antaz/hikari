#include <hikari/view.h>

#include <assert.h>
#include <string.h>

#include <wlr/types/wlr_cursor.h>

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/geometry.h>
#include <hikari/group.h>
#include <hikari/indicator.h>
#include <hikari/layout.h>
#include <hikari/mark.h>
#include <hikari/memory.h>
#include <hikari/operation.h>
#include <hikari/output.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/tile.h>
#include <hikari/view_config.h>
#include <hikari/workspace.h>
#include <hikari/xdg_view.h>
#include <hikari/xwayland_view.h>

#define VIEW(name, link)                                                       \
  static struct hikari_view *name##_view(void)                                 \
  {                                                                            \
    struct wl_list *views = hikari_server.visible_views.link;                  \
                                                                               \
    if (!wl_list_empty(views)) {                                               \
      struct hikari_view *view;                                                \
      view = wl_container_of(views, view, visible_server_views);               \
      return view;                                                             \
    }                                                                          \
                                                                               \
    return NULL;                                                               \
  }                                                                            \
                                                                               \
  static bool is_##name##_view(struct hikari_view *view)                       \
  {                                                                            \
    return view == name##_view();                                              \
  }

VIEW(first, next)
VIEW(last, prev)
#undef VIEW

static void
move_to_top(struct hikari_view *view)
{
  assert(view != NULL);
  assert(hikari_view_is_mapped(view));

  wl_list_remove(&view->sheet_views);
  wl_list_insert(&view->sheet->views, &view->sheet_views);

  wl_list_remove(&view->group_views);
  wl_list_insert(&view->group->views, &view->group_views);

  wl_list_remove(&view->output_views);
  wl_list_insert(&view->output->views, &view->output_views);
}

static void
place_visibly_above(
    struct hikari_view *view, struct hikari_workspace *workspace)
{
  assert(hikari_view_is_forced(view) ? hikari_view_is_hidden(view)
                                     : !hikari_view_is_hidden(view));

  wl_list_remove(&view->visible_server_views);
  wl_list_insert(&hikari_server.visible_views, &view->visible_server_views);

  wl_list_remove(&view->visible_group_views);
  wl_list_insert(&view->group->visible_views, &view->visible_group_views);

  wl_list_remove(&view->group->visible_server_groups);
  wl_list_insert(
      &hikari_server.visible_groups, &view->group->visible_server_groups);

  wl_list_remove(&view->workspace_views);
  wl_list_insert(&workspace->views, &view->workspace_views);
}

static void
raise_view(struct hikari_view *view)
{
  assert(view != NULL);

  move_to_top(view);
  place_visibly_above(view, view->sheet->workspace);
}

static void
refresh_border_geometry(struct hikari_view *view)
{
  assert(view != NULL);
  hikari_border_refresh_geometry(&view->border, view->current_geometry);
  hikari_indicator_frame_refresh_geometry(&view->indicator_frame, view);
}

static inline void
move_view_constrained(
    struct hikari_view *view, struct wlr_box *geometry, int x, int y)
{
  if (!hikari_view_is_hidden(view)) {
    hikari_view_damage_whole(view);
    hikari_indicator_damage(&hikari_server.indicator, view);
  }

  hikari_geometry_constrain_relative(
      geometry, &view->output->usable_area, x, y);
}

static void
move_view(struct hikari_view *view, struct wlr_box *geometry, int x, int y)
{
  if (view->maximized_state != NULL) {
    struct wlr_box *usable_area;

    switch (view->maximized_state->maximization) {
      case HIKARI_MAXIMIZATION_FULLY_MAXIMIZED:
        return;

      case HIKARI_MAXIMIZATION_VERTICALLY_MAXIMIZED:
        usable_area = &view->output->usable_area;

        if (y != usable_area->y) {
          return;
        }

        move_view_constrained(view, geometry, x, y);
        view->geometry.x = x;
        break;

      case HIKARI_MAXIMIZATION_HORIZONTALLY_MAXIMIZED:
        usable_area = &view->output->usable_area;

        if (x != usable_area->x) {
          return;
        }

        move_view_constrained(view, geometry, x, y);
        view->geometry.y = y;
        break;
    }
  } else {
    move_view_constrained(view, geometry, x, y);
  }

#ifdef HAVE_XWAYLAND
  if (view->move != NULL) {
    view->move(view, geometry->x, geometry->y);
  }
#endif

  refresh_border_geometry(view);

  if (!hikari_view_is_hidden(view)) {
    hikari_view_damage_whole(view);
    hikari_indicator_damage(&hikari_server.indicator, view);
  }
}

static void
increase_group_visiblity(struct hikari_view *view)
{
  assert(hikari_view_is_forced(view) ? hikari_view_is_hidden(view)
                                     : !hikari_view_is_hidden(view));

  struct hikari_group *group = view->group;

  if (wl_list_empty(&group->visible_views)) {
    wl_list_insert(
        &hikari_server.visible_groups, &group->visible_server_groups);
  }

  wl_list_init(&view->visible_group_views);
}

static void
decrease_group_visibility(struct hikari_view *view)
{
  struct hikari_group *group = view->group;

  wl_list_remove(&view->visible_group_views);

  if (wl_list_empty(&group->visible_views)) {
    wl_list_remove(&group->visible_server_groups);
  }
}

static void
hide(struct hikari_view *view)
{
  decrease_group_visibility(view);

  wl_list_remove(&view->workspace_views);
  wl_list_init(&view->workspace_views);

  wl_list_remove(&view->visible_server_views);
  wl_list_init(&view->visible_server_views);

  hikari_view_set_hidden(view);
}

static void
detach_from_group(struct hikari_view *view)
{
  struct hikari_group *group = view->group;

  wl_list_remove(&view->group_views);
  wl_list_init(&view->group_views);

  if (wl_list_empty(&group->views)) {
    hikari_group_fini(group);
    hikari_free(group);
  }
}

static void
remove_from_group(struct hikari_view *view)
{
  assert(!hikari_view_is_hidden(view));

  if (!hikari_view_is_hidden(view)) {
    decrease_group_visibility(view);
  }

  detach_from_group(view);
}

static void
cancel_tile(struct hikari_view *view)
{
  if (hikari_view_is_tiling(view)) {
    struct hikari_tile *tile = view->pending_operation.tile;

    hikari_tile_detach(tile);
    hikari_free(tile);
    view->pending_operation.tile = NULL;
  }
}

static inline void
guarded_resize(struct hikari_view *view,
    struct hikari_operation *op,
    void (*f)(struct hikari_view *, struct hikari_operation *))
{
  op->serial = view->resize(view, op->geometry.width, op->geometry.height);

  if (op->serial == 0) {
    f(view, op);
  } else {
    hikari_view_set_dirty(view);
  }
}

static inline void
resize(struct hikari_view *view,
    struct hikari_operation *op,
    void (*f)(struct hikari_view *, struct hikari_operation *))
{
#ifdef HAVE_XWAYLAND
  if (view->move_resize != NULL) {
    op->serial = 0;
    view->move_resize(view,
        op->geometry.x,
        op->geometry.y,
        op->geometry.width,
        op->geometry.height);

    hikari_view_set_dirty(view);
  } else {
    guarded_resize(view, op, f);
  }
#else
  guarded_resize(view, op, f);
#endif
}

static void
commit_pending_geometry(
    struct hikari_view *view, struct wlr_box *pending_geometry)
{
  hikari_view_refresh_geometry(view, pending_geometry);

  hikari_indicator_damage(&hikari_server.indicator, view);
  hikari_view_damage_whole(view);
}

static void
commit_pending_operation(
    struct hikari_view *view, struct hikari_operation *operation)
{
  if (!hikari_view_is_hidden(view)) {
    if (!hikari_view_is_forced(view)) {
      raise_view(view);
    } else {
      move_to_top(view);
    }

    commit_pending_geometry(view, &operation->geometry);

    if (operation->center) {
      hikari_view_center_cursor(view);
      hikari_server_cursor_focus();
    }
  } else {
    hikari_view_refresh_geometry(view, &operation->geometry);

    if (hikari_sheet_is_visible(view->sheet)) {
      if (!hikari_view_is_forced(view)) {
        hikari_view_show(view);
      } else {
        raise_view(view);
      }

      if (operation->center) {
        hikari_view_center_cursor(view);
        hikari_server_cursor_focus();
      }
    } else {
      if (!hikari_view_is_forced(view)) {
        move_to_top(view);
      } else {
        raise_view(view);
      }
    }
  }
}

static void
commit_reset(struct hikari_view *view, struct hikari_operation *operation)
{
  hikari_indicator_damage(&hikari_server.indicator, view);

  if (hikari_view_is_tiled(view)) {
    assert(!hikari_tile_is_attached(view->tile));
    hikari_free(view->tile);
    view->tile = NULL;
  }

  if (hikari_view_is_maximized(view)) {
    hikari_maximized_state_destroy(view->maximized_state);
    view->maximized_state = NULL;

    if (!view->use_csd) {
      if (view == hikari_server.workspace->focus_view) {
        view->border.state = HIKARI_BORDER_ACTIVE;
      } else {
        view->border.state = HIKARI_BORDER_INACTIVE;
      }
    }
  }

  commit_pending_operation(view, operation);
}

static void
queue_reset(struct hikari_view *view, bool center)
{
  struct wlr_box *view_geometry = hikari_view_geometry(view);
  struct hikari_operation *op = &view->pending_operation;
  struct hikari_tile *tile = view->tile;
  struct wlr_box *geometry = &view->geometry;

  cancel_tile(view);

  if (hikari_view_is_tiled(view) && hikari_tile_is_attached(tile)) {
    hikari_tile_detach(tile);
  }

  op->type = HIKARI_OPERATION_TYPE_RESET;
  op->center = center;
  memcpy(&op->geometry, geometry, sizeof(struct wlr_box));

  if (view_geometry->width == geometry->width &&
      view_geometry->height == geometry->height) {
    hikari_view_set_dirty(view);
    hikari_view_commit_pending_operation(view, view_geometry);
  } else {
    resize(view, op, commit_reset);
  }

  assert(
      hikari_view_is_tiled(view) ? !hikari_tile_is_attached(view->tile) : true);
}

static void
clear_focus(struct hikari_view *view)
{
  if (hikari_view_is_focus_view(view)) {
    if (hikari_view_has_focus(view)) {
      assert(!hikari_server_in_lock_mode());

      if (!hikari_server_in_normal_mode()) {
        hikari_server_enter_normal_mode(NULL);
      }

      hikari_workspace_focus_view(hikari_server.workspace, NULL);
    } else {
      view->sheet->workspace->focus_view = NULL;
    }
  }
}

void
hikari_view_init(
    struct hikari_view *view, bool child, struct hikari_workspace *workspace)
{
#if !defined(NDEBUG)
  printf("VIEW INIT %p\n", view);
#endif
  view->flags = hikari_view_hidden_flag;
  view->border.state = HIKARI_BORDER_INACTIVE;
  view->sheet = NULL;
  view->mark = NULL;
  view->surface = NULL;
  view->maximized_state = NULL;
  view->output = NULL;
  view->group = NULL;
  view->title = NULL;
  view->tile = NULL;
  view->id = NULL;
  view->use_csd = false;
  view->child = child;
  view->current_geometry = &view->geometry;
  view->current_unmaximized_geometry = &view->geometry;

  hikari_view_unset_dirty(view);
  view->pending_operation.tile = NULL;

  wl_list_init(&view->children);
}

void
hikari_view_fini(struct hikari_view *view)
{
  assert(hikari_view_is_hidden(view));
  assert(!hikari_view_is_mapped(view));
  assert(!hikari_view_is_forced(view));

#if !defined(NDEBUG)
  printf("DESTROY VIEW %p\n", view);
#endif

  hikari_free(view->title);
  hikari_free(view->id);

  if (view->group != NULL) {
    detach_from_group(view);
  }

  if (view->sheet != NULL) {
    wl_list_remove(&view->sheet_views);
    wl_list_remove(&view->output_views);
  }

  if (view->maximized_state != NULL) {
    hikari_maximized_state_destroy(view->maximized_state);
  }

  if (view->mark != NULL) {
    hikari_mark_clear(view->mark);
  }

  if (hikari_view_is_tiled(view)) {
    struct hikari_tile *tile = view->tile;
    hikari_tile_detach(tile);
    hikari_free(tile);
    view->tile = NULL;
  }

  cancel_tile(view);
}

void
hikari_view_set_title(struct hikari_view *view, const char *title)
{
  hikari_free(view->title);

  if (title != NULL) {
    view->title = hikari_malloc(strlen(title) + 1);
    strcpy(view->title, title);

    if (hikari_server.workspace->focus_view == view) {
      assert(!hikari_view_is_hidden(view));
      struct hikari_output *output = view->output;
      hikari_indicator_update_title(&hikari_server.indicator, output, title);
    }
  } else {
    view->title = NULL;
  }
}

static void
set_app_id(struct hikari_view *view, const char *id)
{
  assert(view->id == NULL);
  assert(id != NULL);

  view->id = hikari_malloc(strlen(id) + 1);

  strcpy(view->id, id);
}

struct hikari_damage_data {
  struct wlr_box *geometry;
  struct wlr_surface *surface;

  struct hikari_view *view;
  struct hikari_output *output;

  bool whole;
};

static void
damage_whole_surface(struct wlr_surface *surface, int sx, int sy, void *data)
{
  struct hikari_damage_data *damage_data = data;
  struct hikari_output *output = damage_data->output;
  struct hikari_view *view = damage_data->view;

  if (view->surface == surface) {
    hikari_view_damage_border(view);
  } else {
    struct wlr_box geometry;
    memcpy(&geometry, damage_data->geometry, sizeof(struct wlr_box));

    geometry.x += sx;
    geometry.y += sy;
    geometry.width = surface->current.width;
    geometry.height = surface->current.height;

    hikari_output_add_damage(output, &geometry);
  }
}

void
hikari_view_damage_whole(struct hikari_view *view)
{
  assert(view != NULL);

  struct hikari_output *output = view->output;

  // TODO I know, this needs to be done A LOT better
  if (view->use_csd) {
    hikari_output_damage_whole(output);
    return;
  }

  struct hikari_damage_data damage_data;

  damage_data.geometry = hikari_view_geometry(view);
  damage_data.output = output;
  damage_data.view = view;

  hikari_node_for_each_surface(
      (struct hikari_node *)view, damage_whole_surface, &damage_data);
}

static struct wlr_box *
refresh_unmaximized_geometry(struct hikari_view *view)
{
  assert(view != NULL);

  if (hikari_view_is_tiled(view)) {
    return &view->tile->view_geometry;
  } else {
    return &view->geometry;
  }
}

static struct wlr_box *
refresh_geometry(struct hikari_view *view)
{
  assert(view != NULL);

  if (view->maximized_state != NULL) {
    return &view->maximized_state->geometry;
  } else {
    return refresh_unmaximized_geometry(view);
  }
}

static inline int
constrain_size(int min, int max, int value)
{
  if (value > max) {
    return max;
  } else if (value < min) {
    return min;
  } else {
    return value;
  }
}

static void
commit_resize(struct hikari_view *view, struct hikari_operation *operation)
{
  commit_pending_operation(view, operation);
}

static void
queue_resize(struct hikari_view *view,
    struct wlr_box *geometry,
    int requested_x,
    int requested_y,
    int requested_width,
    int requested_height)
{
  struct hikari_operation *op = &view->pending_operation;

  int min_width;
  int min_height;
  int max_width;
  int max_height;

  int new_width;
  int new_height;

  if (view->maximized_state != NULL) {
    switch (view->maximized_state->maximization) {
      case HIKARI_MAXIMIZATION_FULLY_MAXIMIZED:
        return;

      case HIKARI_MAXIMIZATION_VERTICALLY_MAXIMIZED:
        view->constraints(
            view, &min_width, &min_height, &max_width, &max_height);
        new_width = constrain_size(min_width, max_width, requested_width);
        new_height = geometry->height;
        view->geometry.width = new_width;
        break;

      case HIKARI_MAXIMIZATION_HORIZONTALLY_MAXIMIZED:
        view->constraints(
            view, &min_width, &min_height, &max_width, &max_height);
        new_width = geometry->width;
        new_height = constrain_size(min_height, max_height, requested_height);
        view->geometry.height = new_height;
        break;
    }
  } else {
    view->constraints(view, &min_width, &min_height, &max_width, &max_height);
    new_width = constrain_size(min_width, max_width, requested_width);
    new_height = constrain_size(min_height, max_height, requested_height);
  }

  struct hikari_output *output = view->output;

  op->type = HIKARI_OPERATION_TYPE_RESIZE;
  op->geometry.width = new_width;
  op->geometry.height = new_height;
  op->center = false;

  hikari_geometry_constrain_relative(
      &op->geometry, &output->usable_area, requested_x, requested_y);

  resize(view, op, commit_resize);
}

void
hikari_view_resize(struct hikari_view *view, int dwidth, int dheight)
{
  assert(view != NULL);
  assert(view->resize != NULL);
  assert(view->constraints != NULL);

  if (hikari_view_is_dirty(view)) {
    return;
  }

  struct wlr_box *geometry = hikari_view_geometry(view);

  int requested_width = geometry->width + dwidth;
  int requested_height = geometry->height + dheight;

  queue_resize(view,
      geometry,
      geometry->x,
      geometry->y,
      requested_width,
      requested_height);
}

void
hikari_view_resize_absolute(struct hikari_view *view, int width, int height)
{
  assert(view != NULL);
  assert(view->resize != NULL);
  assert(view->constraints != NULL);

  if (hikari_view_is_dirty(view)) {
    return;
  }

  struct wlr_box *geometry = hikari_view_geometry(view);
  queue_resize(view, geometry, geometry->x, geometry->y, width, height);
}

void
hikari_view_move_resize(
    struct hikari_view *view, int x, int y, int width, int height)
{
  assert(view != NULL);
  assert(view->resize != NULL);
  assert(view->constraints != NULL);

  if (hikari_view_is_dirty(view)) {
    return;
  }

  struct wlr_box *geometry = hikari_view_geometry(view);

  int requested_x = geometry->x + x;
  int requested_y = geometry->y + y;
  int requested_width = geometry->width + width;
  int requested_height = geometry->height + height;

  queue_resize(view,
      geometry,
      requested_x,
      requested_y,
      requested_width,
      requested_height);
}

void
hikari_view_move(struct hikari_view *view, int x, int y)
{
  assert(view != NULL);

  struct wlr_box *geometry = hikari_view_geometry(view);

  move_view(view, geometry, geometry->x + x, geometry->y + y);
}

void
hikari_view_move_absolute(struct hikari_view *view, int x, int y)
{
  assert(view != NULL);

  struct wlr_box *geometry = hikari_view_geometry(view);

  move_view(view, geometry, x, y);
}

#define MOVE(pos)                                                              \
  void hikari_view_move_##pos(struct hikari_view *view)                        \
  {                                                                            \
    assert(view != NULL);                                                      \
                                                                               \
    struct hikari_output *output = view->output;                               \
    struct wlr_box *usable_area = &output->usable_area;                        \
    struct wlr_box *border_geometry = hikari_view_border_geometry(view);       \
    struct wlr_box *geometry = hikari_view_geometry(view);                     \
                                                                               \
    int x;                                                                     \
    int y;                                                                     \
    hikari_geometry_position_##pos(border_geometry, usable_area, &x, &y);      \
                                                                               \
    move_view(view, geometry, x, y);                                           \
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

static void
new_subsurface_handler(struct wl_listener *listener, void *data)
{
  struct hikari_view *view = wl_container_of(listener, view, new_subsurface);

  struct wlr_subsurface *wlr_subsurface = data;

  struct hikari_view_subsurface *view_subsurface =
      hikari_malloc(sizeof(struct hikari_view_subsurface));

  hikari_view_subsurface_init(view_subsurface, view, wlr_subsurface);
}

void
hikari_view_map(struct hikari_view *view, struct wlr_surface *surface)
{
  assert(hikari_view_is_hidden(view));
  assert(!hikari_view_is_unmanaged(view));
  assert(!hikari_view_is_mapped(view));

  struct hikari_sheet *sheet = view->sheet;
  struct hikari_output *output = view->output;
  struct hikari_group *group;
  bool focus;

  struct hikari_view_config *view_config =
      hikari_configuration_resolve_view_config(hikari_configuration, view->id);

  view->surface = surface;

  view->new_subsurface.notify = new_subsurface_handler;
  wl_signal_add(&surface->events.new_subsurface, &view->new_subsurface);

  struct wlr_subsurface *wlr_subsurface;
  wl_list_for_each (
      wlr_subsurface, &surface->current.subsurfaces_below, current.link) {
    struct hikari_view_subsurface *subsurface =
        (struct hikari_view_subsurface *)malloc(
            sizeof(struct hikari_view_subsurface));
    hikari_view_subsurface_init(subsurface, view, wlr_subsurface);
  }
  wl_list_for_each (
      wlr_subsurface, &surface->current.subsurfaces_above, current.link) {
    struct hikari_view_subsurface *subsurface =
        (struct hikari_view_subsurface *)malloc(
            sizeof(struct hikari_view_subsurface));
    hikari_view_subsurface_init(subsurface, view, wlr_subsurface);
  }

  if (view_config != NULL) {
    struct hikari_mark *mark;
    struct hikari_view_properties *properties =
        hikari_view_config_resolve_properties(view_config, view->child);

    assert(properties != NULL);

    group = hikari_view_properties_resolve_group(properties, view->id);
    mark = properties->mark;

    if (mark != NULL && mark->view == NULL) {
      hikari_mark_set(mark, view);
    }

    focus = properties->focus;
  } else {
    group = hikari_server_find_or_create_group(view->id);
    focus = false;
  }

  view->group = group;

  wl_list_insert(&sheet->views, &view->sheet_views);
  wl_list_insert(&group->views, &view->group_views);
  wl_list_insert(&output->views, &view->output_views);

  if (!hikari_server_in_lock_mode() || hikari_view_is_public(view)) {
    hikari_view_show(view);

    if (focus) {
      hikari_view_center_cursor(view);
    }

    hikari_server_cursor_focus();
  } else {
    hikari_view_set_forced(view);
    increase_group_visiblity(view);
    raise_view(view);
  }
}

void
hikari_view_unmap(struct hikari_view *view)
{
  assert(!hikari_view_is_unmanaged(view));
  assert(hikari_view_is_mapped(view));

  wl_list_remove(&view->new_subsurface.link);

  struct hikari_view_child *child, *child_temp;
  wl_list_for_each_safe (child, child_temp, &view->children, link) {
    struct hikari_view_subsurface *subsurface =
        (struct hikari_view_subsurface *)child;
    hikari_view_subsurface_fini(subsurface);
    hikari_free(subsurface);
  }

  if (hikari_view_is_forced(view)) {
    if (hikari_view_is_hidden(view)) {
      hide(view);
    } else {
      hikari_view_damage_whole(view);
      hikari_view_set_hidden(view);
    }

    hikari_view_unset_forced(view);
  }

  if (!hikari_view_is_hidden(view)) {
    hikari_view_hide(view);
    hikari_server_cursor_focus();
  }

  assert(hikari_view_is_hidden(view));
  assert(!hikari_view_is_forced(view));

  view->surface = NULL;

  struct hikari_mark *mark = view->mark;
  if (mark != NULL) {
    hikari_mark_clear(mark);
  }

  detach_from_group(view);
  view->group = NULL;

  cancel_tile(view);

  if (hikari_view_is_tiled(view)) {
    struct wlr_box geometry;
    struct hikari_tile *tile = view->tile;

    memcpy(&geometry, hikari_view_geometry(view), sizeof(struct wlr_box));

    if (hikari_tile_is_attached(tile)) {
      hikari_tile_detach(tile);
    }

    hikari_free(tile);
    view->tile = NULL;

    hikari_view_refresh_geometry(view, &geometry);
  }

  wl_list_remove(&view->sheet_views);
  wl_list_init(&view->sheet_views);

  wl_list_remove(&view->output_views);
  wl_list_init(&view->output_views);

  hikari_view_unset_dirty(view);

  assert(!hikari_view_is_tiling(view));
  assert(!hikari_view_is_tiled(view));
}

void
hikari_view_show(struct hikari_view *view)
{
  assert(view != NULL);
  assert(hikari_view_is_hidden(view));
  assert(!hikari_view_is_forced(view));

#if !defined(NDEBUG)
  printf("SHOW %p\n", view);
#endif
  hikari_view_unset_hidden(view);

  increase_group_visiblity(view);

  raise_view(view);

  hikari_view_damage_whole(view);

  assert(is_first_view(view));
}

void
hikari_view_hide(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));
  assert(!hikari_view_is_forced(view));

#if !defined(NDEBUG)
  printf("HIDE %p\n", view);
#endif

  clear_focus(view);
  hide(view);

  hikari_view_damage_whole(view);
}

void
hikari_view_raise(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));

  if (is_first_view(view)) {
    return;
  }

  raise_view(view);
  hikari_view_damage_whole(view);
}

void
hikari_view_lower(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));

  if (is_last_view(view)) {
    return;
  }

  wl_list_remove(&view->sheet_views);
  wl_list_insert(view->sheet->views.prev, &view->sheet_views);

  wl_list_remove(&view->group_views);
  wl_list_insert(view->group->views.prev, &view->group_views);

  wl_list_remove(&view->output_views);
  wl_list_insert(view->output->views.prev, &view->output_views);

  wl_list_remove(&view->visible_group_views);
  wl_list_insert(view->group->visible_views.prev, &view->visible_group_views);

  wl_list_remove(&view->group->visible_server_groups);
  wl_list_insert(
      hikari_server.visible_groups.prev, &view->group->visible_server_groups);

  wl_list_remove(&view->workspace_views);
  wl_list_insert(view->sheet->workspace->views.prev, &view->workspace_views);

  wl_list_remove(&view->visible_server_views);
  wl_list_insert(hikari_server.visible_views.prev, &view->visible_server_views);

  hikari_view_damage_whole(view);
}

static void
commit_tile(struct hikari_view *view, struct hikari_operation *operation)
{
  if (view->maximized_state) {
    hikari_maximized_state_destroy(view->maximized_state);
    view->maximized_state = NULL;
    if (!view->use_csd) {
      view->border.state = HIKARI_BORDER_INACTIVE;
    }
  }

  assert(hikari_tile_is_attached(operation->tile));

  if (hikari_view_is_tiled(view)) {
    struct hikari_tile *tile = view->tile;

    assert(hikari_tile_is_attached(tile));

    wl_list_remove(&tile->layout_tiles);
    hikari_free(tile);
    view->tile = NULL;
  }

  assert(!hikari_view_is_tiled(view));
  assert(operation->tile != NULL);
  view->tile = operation->tile;
  operation->tile = NULL;

  if (!hikari_view_is_hidden(view)) {
    commit_pending_geometry(view, &operation->geometry);
    if (operation->center) {
      hikari_view_center_cursor(view);
    }
    hikari_server_cursor_focus();
  } else {
    hikari_view_refresh_geometry(view, &operation->geometry);
  }
}

static void
queue_tile(struct hikari_view *view,
    struct hikari_layout *layout,
    struct hikari_tile *tile,
    bool center)
{
  assert(!hikari_view_is_dirty(view));

  struct hikari_operation *op = &view->pending_operation;

  struct wlr_box *current_geometry = hikari_view_geometry(view);

  op->type = HIKARI_OPERATION_TYPE_TILE;
  op->tile = tile;
  op->geometry = tile->view_geometry;
  op->center = center;

  if (current_geometry->width == op->geometry.width &&
      current_geometry->height == op->geometry.height) {
    hikari_view_set_dirty(view);

    hikari_view_commit_pending_operation(view, current_geometry);
  } else {
    resize(view, op, commit_tile);
  }
}

void
hikari_view_tile(
    struct hikari_view *view, struct wlr_box *geometry, bool center)
{
  assert(!hikari_view_is_dirty(view));
  assert(hikari_view_is_tileable(view));

  struct hikari_layout *layout = view->sheet->workspace->sheet->layout;

  struct hikari_tile *tile = hikari_malloc(sizeof(struct hikari_tile));
  hikari_tile_init(tile, view, layout, geometry, geometry);

  queue_tile(view, layout, tile, center);

  wl_list_insert(layout->tiles.prev, &tile->layout_tiles);
}

static void
commit_full_maximize(
    struct hikari_view *view, struct hikari_operation *operation)
{
  if (!view->maximized_state) {
    view->maximized_state =
        hikari_malloc(sizeof(struct hikari_maximized_state));
  }

  view->maximized_state->maximization = HIKARI_MAXIMIZATION_FULLY_MAXIMIZED;
  view->maximized_state->geometry = operation->geometry;

  if (!view->use_csd) {
    view->border.state = HIKARI_BORDER_NONE;
  }

  commit_pending_operation(view, operation);
}

static void
queue_full_maximize(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));

  struct hikari_operation *op = &view->pending_operation;
  struct hikari_output *output = view->output;

  op->type = HIKARI_OPERATION_TYPE_FULL_MAXIMIZE;
  op->geometry = output->usable_area;
  op->center = true;

  resize(view, op, commit_full_maximize);
}

static void
commit_unmaximize(struct hikari_view *view, struct hikari_operation *operation)
{
  hikari_view_damage_whole(view);

  hikari_free(view->maximized_state);
  view->maximized_state = NULL;

  if (!view->use_csd) {
    view->border.state = HIKARI_BORDER_ACTIVE;
  }

  commit_pending_operation(view, operation);
}

static void
queue_unmaximize(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));

  struct hikari_operation *op = &view->pending_operation;

  op->type = HIKARI_OPERATION_TYPE_UNMAXIMIZE;
  op->center = true;

  if (view->tile != NULL) {
    op->geometry = view->tile->view_geometry;
  } else {
    op->geometry = view->geometry;
  }

  resize(view, op, commit_unmaximize);
}

void
hikari_view_toggle_full_maximize(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));

  if (hikari_view_is_dirty(view)) {
    return;
  }

  if (hikari_view_is_fully_maximized(view)) {
    queue_unmaximize(view);
  } else {
    queue_full_maximize(view);
  }
}

void
hikari_view_toggle_public(struct hikari_view *view)
{
  if (hikari_view_is_public(view)) {
    hikari_view_unset_public(view);
  } else {
    hikari_view_set_public(view);
  }
}

static void
commit_horizontal_maximize(
    struct hikari_view *view, struct hikari_operation *operation)
{
  if (!view->maximized_state) {
    view->maximized_state =
        hikari_malloc(sizeof(struct hikari_maximized_state));
  } else {
    switch (view->maximized_state->maximization) {
      case HIKARI_MAXIMIZATION_HORIZONTALLY_MAXIMIZED:
        commit_full_maximize(view, operation);
        return;

      case HIKARI_MAXIMIZATION_FULLY_MAXIMIZED:
        if (!view->use_csd) {
          view->border.state = HIKARI_BORDER_INACTIVE;
        }
        break;

      case HIKARI_MAXIMIZATION_VERTICALLY_MAXIMIZED:
        assert(false);
        break;
    }
  }

  view->maximized_state->maximization =
      HIKARI_MAXIMIZATION_HORIZONTALLY_MAXIMIZED;
  view->maximized_state->geometry = operation->geometry;

  commit_pending_operation(view, operation);
}

static void
queue_horizontal_maximize(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));

  struct hikari_operation *op = &view->pending_operation;
  struct hikari_output *output = view->output;

  struct wlr_box *geometry = view->current_unmaximized_geometry;

  op->type = HIKARI_OPERATION_TYPE_HORIZONTAL_MAXIMIZE;
  op->geometry.x = output->usable_area.x;
  op->geometry.y = geometry->y;
  op->geometry.height = geometry->height;
  op->geometry.width = output->usable_area.width;
  op->center = true;

  resize(view, op, commit_horizontal_maximize);
}

static void
commit_vertical_maximize(
    struct hikari_view *view, struct hikari_operation *operation)
{
  if (!view->maximized_state) {
    view->maximized_state =
        hikari_malloc(sizeof(struct hikari_maximized_state));
  } else {
    switch (view->maximized_state->maximization) {
      case HIKARI_MAXIMIZATION_HORIZONTALLY_MAXIMIZED:
        commit_full_maximize(view, operation);
        return;

      case HIKARI_MAXIMIZATION_FULLY_MAXIMIZED:
        if (!view->use_csd) {
          view->border.state = HIKARI_BORDER_INACTIVE;
        }
        break;

      case HIKARI_MAXIMIZATION_VERTICALLY_MAXIMIZED:
        assert(false);
        break;
    }
  }

  view->maximized_state->maximization =
      HIKARI_MAXIMIZATION_VERTICALLY_MAXIMIZED;
  view->maximized_state->geometry = operation->geometry;

  commit_pending_operation(view, operation);
}

static void
queue_vertical_maximize(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));

  struct hikari_operation *op = &view->pending_operation;
  struct hikari_output *output = view->output;

  struct wlr_box *geometry = view->current_unmaximized_geometry;

  op->type = HIKARI_OPERATION_TYPE_VERTICAL_MAXIMIZE;
  op->geometry.x = geometry->x;
  op->geometry.y = output->usable_area.y;
  op->geometry.height = output->usable_area.height;
  op->geometry.width = geometry->width;
  op->center = true;

  resize(view, op, commit_vertical_maximize);
}

void
hikari_view_toggle_vertical_maximize(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));

  if (hikari_view_is_dirty(view)) {
    return;
  }

  if (view->maximized_state != NULL) {
    switch (view->maximized_state->maximization) {
      case HIKARI_MAXIMIZATION_FULLY_MAXIMIZED:
        queue_horizontal_maximize(view);
        break;

      case HIKARI_MAXIMIZATION_VERTICALLY_MAXIMIZED:
        queue_unmaximize(view);
        break;

      case HIKARI_MAXIMIZATION_HORIZONTALLY_MAXIMIZED:
        queue_full_maximize(view);
        break;
    }
  } else {
    queue_vertical_maximize(view);
  }
}

void
hikari_view_toggle_horizontal_maximize(struct hikari_view *view)
{
  assert(view != NULL);
  assert(!hikari_view_is_hidden(view));

  if (view->maximized_state != NULL) {
    switch (view->maximized_state->maximization) {
      case HIKARI_MAXIMIZATION_FULLY_MAXIMIZED:
        queue_vertical_maximize(view);
        break;

      case HIKARI_MAXIMIZATION_VERTICALLY_MAXIMIZED:
        queue_full_maximize(view);
        break;

      case HIKARI_MAXIMIZATION_HORIZONTALLY_MAXIMIZED:
        queue_unmaximize(view);
        break;
    }
  } else {
    queue_horizontal_maximize(view);
  }
}

void
hikari_view_toggle_floating(struct hikari_view *view)
{
  if (!hikari_view_is_floating(view)) {
    if (hikari_view_is_tiled(view)) {
      hikari_view_reset_geometry(view);
    }
    hikari_view_set_floating(view);
  } else {
    hikari_view_unset_floating(view);
  }
}

void
hikari_view_reset_geometry(struct hikari_view *view)
{
  queue_reset(view, true);
}

void
hikari_view_evacuate(struct hikari_view *view, struct hikari_sheet *sheet)
{
#ifndef NDEBUG
  printf("EVACUATE VIEW %p\n", view);
#endif

  clear_focus(view);

  view->output = sheet->workspace->output;
  view->sheet = sheet;

  if (!hikari_view_is_hidden(view)) {
    if (hikari_view_is_forced(view)) {
      move_to_top(view);
    } else {
      raise_view(view);
    }

    if (hikari_sheet_is_visible(sheet)) {
      hikari_view_damage_whole(view);
      hikari_indicator_damage(&hikari_server.indicator, view);
    } else {
      if (hikari_view_is_forced(view)) {
        move_to_top(view);
      } else {
        hikari_view_hide(view);
      }
    }
  } else {
    if (hikari_view_is_forced(view)) {
      raise_view(view);
    } else {
      move_to_top(view);
    }
  }

  if (hikari_view_is_tiled(view) || hikari_view_is_maximized(view)) {
    queue_reset(view, false);
  }
}

void
hikari_view_pin_to_sheet(struct hikari_view *view, struct hikari_sheet *sheet)
{
  assert(view != NULL);
  assert(sheet != NULL);
  assert(sheet->workspace->output == view->output);

  if (view->sheet == sheet) {
    assert(!hikari_view_is_hidden(view));

    if (view->sheet->workspace->sheet != sheet && sheet->nr != 0) {
      hikari_view_hide(view);
      hikari_server_cursor_focus();

      move_to_top(view);
    } else {
      hikari_view_raise(view);
      hikari_indicator_damage(&hikari_server.indicator, view);
    }
  } else {
    if (hikari_sheet_is_visible(sheet)) {
      place_visibly_above(view, sheet->workspace);

      hikari_view_damage_whole(view);
      hikari_indicator_damage(&hikari_server.indicator, view);
    } else {
      hikari_view_hide(view);
      hikari_server_cursor_focus();
    }

    view->sheet = sheet;

    if (hikari_view_is_tiled(view)) {
      queue_reset(view, true);
    } else {
      move_to_top(view);
    }
  }
}

void
hikari_view_center_cursor(struct hikari_view *view)
{
  assert(view != NULL);

  struct hikari_output *output = view->output;
  struct wlr_box *view_geometry = hikari_view_geometry(view);

  struct wlr_box geometry;
  hikari_geometry_constrain_size(
      view_geometry, &output->usable_area, &geometry);

  hikari_cursor_center(&hikari_server.cursor, output, &geometry);
}

void
hikari_view_top_left_cursor(struct hikari_view *view)
{
  assert(view != NULL);

  struct wlr_box *geometry = hikari_view_geometry(view);
  struct hikari_output *output = view->output;

  int x = output->geometry.x + geometry->x;
  int y = output->geometry.y + geometry->y;

  hikari_cursor_warp(&hikari_server.cursor, x, y);
}

void
hikari_view_bottom_right_cursor(struct hikari_view *view)
{
  assert(view != NULL);

  struct wlr_box *geometry = hikari_view_geometry(view);
  struct hikari_output *output = view->output;

  int x = output->geometry.x + geometry->x + geometry->width;
  int y = output->geometry.y + geometry->y + geometry->height;

  hikari_cursor_warp(&hikari_server.cursor, x, y);
}

void
hikari_view_toggle_invisible(struct hikari_view *view)
{
  if (hikari_view_is_invisible(view)) {
    hikari_view_unset_invisible(view);
  } else {
    if (hikari_view_is_tiled(view)) {
      hikari_view_reset_geometry(view);
    }
    hikari_view_set_invisible(view);
  }
}

void
hikari_view_group(struct hikari_view *view, struct hikari_group *group)
{
  assert(view != NULL);
  assert(group != NULL);
  assert(view->group != NULL);

  if (view->group == group) {
    return;
  }

  remove_from_group(view);
  view->group = group;

  increase_group_visiblity(view);

  raise_view(view);

  hikari_view_damage_whole(view);
}

void
hikari_view_exchange(struct hikari_view *from, struct hikari_view *to)
{
  assert(from != NULL);
  assert(to != NULL);

  if (hikari_view_is_dirty(from) || hikari_view_is_dirty(to)) {
    return;
  }

  assert(from->tile != NULL);
  assert(to->tile != NULL);
  assert(to->tile->view->sheet == from->tile->view->sheet);

  struct hikari_layout *layout = from->sheet->workspace->sheet->layout;

  struct wlr_box *from_geometry = &from->tile->tile_geometry;
  struct wlr_box *to_geometry = &to->tile->tile_geometry;

  struct hikari_tile *from_tile = hikari_malloc(sizeof(struct hikari_tile));
  struct hikari_tile *to_tile = hikari_malloc(sizeof(struct hikari_tile));

  hikari_tile_init(from_tile, from, layout, to_geometry, to_geometry);
  hikari_tile_init(to_tile, to, layout, from_geometry, from_geometry);

  wl_list_insert(&from->tile->layout_tiles, &to_tile->layout_tiles);
  wl_list_insert(&to->tile->layout_tiles, &from_tile->layout_tiles);

  queue_tile(from, layout, from_tile, true);
  queue_tile(to, layout, to_tile, false);
}

static void
destroy_subsurface_handler(struct wl_listener *listener, void *data)
{
  struct hikari_view_subsurface *view_subsurface =
      wl_container_of(listener, view_subsurface, destroy);

  hikari_view_subsurface_fini(view_subsurface);

  hikari_free(view_subsurface);
}

void
hikari_view_subsurface_init(struct hikari_view_subsurface *view_subsurface,
    struct hikari_view *parent,
    struct wlr_subsurface *subsurface)
{
  view_subsurface->subsurface = subsurface;

  view_subsurface->destroy.notify = destroy_subsurface_handler;
  wl_signal_add(
      &subsurface->surface->events.destroy, &view_subsurface->destroy);

  hikari_view_child_init(
      (struct hikari_view_child *)view_subsurface, parent, subsurface->surface);
}

void
hikari_view_child_fini(struct hikari_view_child *view_child)
{
  wl_list_remove(&view_child->link);
  wl_list_remove(&view_child->commit.link);
  wl_list_remove(&view_child->new_subsurface.link);
}

void
hikari_view_subsurface_fini(struct hikari_view_subsurface *view_subsurface)
{
  hikari_view_child_fini(&view_subsurface->view_child);
  wl_list_remove(&view_subsurface->destroy.link);
}

static void
damage_surface(struct wlr_surface *surface, int sx, int sy, void *data)
{
  struct hikari_damage_data *damage_data = data;
  struct hikari_output *output = damage_data->output;

  if (damage_data->whole) {
    damage_whole_surface(surface, sx, sy, data);
  } else {
    struct wlr_box *geometry = damage_data->geometry;

    hikari_output_add_effective_surface_damage(
        output, surface, geometry->x + sx, geometry->y + sy);
  }
}

static void
damage_single_surface(struct wlr_surface *surface, int sx, int sy, void *data)
{
  struct hikari_damage_data *damage_data = data;

  if (damage_data->surface == surface) {
    damage_surface(surface, sx, sy, data);
  }
}

static void
commit_child_handler(struct wl_listener *listener, void *data)
{
  struct hikari_view_child *view_child =
      wl_container_of(listener, view_child, commit);

  struct hikari_view *parent = view_child->parent;

  if (!hikari_view_is_hidden(parent)) {
    struct wlr_surface *surface = view_child->surface;

    hikari_view_damage_surface(parent, surface, false);
  }
}

static void
view_subsurface_create(
    struct wlr_subsurface *wlr_subsurface, struct hikari_view *parent)
{
  struct hikari_view_subsurface *view_subsurface =
      hikari_malloc(sizeof(struct hikari_view_subsurface));

  hikari_view_subsurface_init(view_subsurface, parent, wlr_subsurface);
}

static void
new_subsurface_child_handler(struct wl_listener *listener, void *data)
{
  struct hikari_view_child *view_child =
      wl_container_of(listener, view_child, new_subsurface);

  struct wlr_subsurface *wlr_subsurface = data;

  view_subsurface_create(wlr_subsurface, view_child->parent);
}

void
hikari_view_child_init(struct hikari_view_child *view_child,
    struct hikari_view *parent,
    struct wlr_surface *surface)
{
  view_child->parent = parent;
  view_child->surface = surface;

  view_child->new_subsurface.notify = new_subsurface_child_handler;
  wl_signal_add(&surface->events.new_subsurface, &view_child->new_subsurface);

  view_child->commit.notify = commit_child_handler;
  wl_signal_add(&surface->events.commit, &view_child->commit);

  wl_list_insert(&parent->children, &view_child->link);

  struct wlr_subsurface *subsurface;
  wl_list_for_each (
      subsurface, &surface->current.subsurfaces_below, current.link) {
    view_subsurface_create(subsurface, parent);
  }
  wl_list_for_each (
      subsurface, &surface->current.subsurfaces_above, current.link) {
    view_subsurface_create(subsurface, parent);
  }
}

void
hikari_view_damage_surface(
    struct hikari_view *view, struct wlr_surface *surface, bool whole)
{
  assert(view != NULL);

  // TODO I know, this needs to be done A LOT better
  if (view->use_csd) {
    hikari_output_damage_whole(view->output);
    return;
  }

  struct hikari_damage_data damage_data;

  damage_data.geometry = hikari_view_geometry(view);
  damage_data.output = view->output;
  damage_data.surface = surface;
  damage_data.whole = whole;
  damage_data.view = view;

  hikari_node_for_each_surface(
      (struct hikari_node *)view, damage_single_surface, &damage_data);
}

void
hikari_view_refresh_geometry(struct hikari_view *view, struct wlr_box *geometry)
{
  struct wlr_box *new_geometry = refresh_geometry(view);

  memcpy(new_geometry, geometry, sizeof(struct wlr_box));

  view->current_geometry = new_geometry;
  view->current_unmaximized_geometry = refresh_unmaximized_geometry(view);

  refresh_border_geometry(view);
}

static void
commit_operation(struct hikari_operation *operation, struct hikari_view *view)
{
  switch (operation->type) {
    case HIKARI_OPERATION_TYPE_RESIZE:
      commit_resize(view, operation);
      break;

    case HIKARI_OPERATION_TYPE_RESET:
      commit_reset(view, operation);
      break;

    case HIKARI_OPERATION_TYPE_UNMAXIMIZE:
      commit_unmaximize(view, operation);
      break;

    case HIKARI_OPERATION_TYPE_FULL_MAXIMIZE:
      commit_full_maximize(view, operation);
      break;

    case HIKARI_OPERATION_TYPE_VERTICAL_MAXIMIZE:
      commit_vertical_maximize(view, operation);
      break;

    case HIKARI_OPERATION_TYPE_HORIZONTAL_MAXIMIZE:
      commit_horizontal_maximize(view, operation);
      break;

    case HIKARI_OPERATION_TYPE_TILE:
      commit_tile(view, operation);
      break;
  }
}

void
hikari_view_commit_pending_operation(
    struct hikari_view *view, struct wlr_box *geometry)
{
  assert(view != NULL);
  assert(hikari_view_is_dirty(view));

  view->pending_operation.geometry.width = geometry->width;
  view->pending_operation.geometry.height = geometry->height;

  hikari_indicator_damage(&hikari_server.indicator, view);
  hikari_view_damage_whole(view);

  commit_operation(&view->pending_operation, view);
  hikari_view_unset_dirty(view);
}

void
hikari_view_activate(struct hikari_view *view, bool active)
{
  assert(view != NULL);

  if (view->activate) {
    if (view->border.state != HIKARI_BORDER_NONE) {
      view->border.state =
          active ? HIKARI_BORDER_ACTIVE : HIKARI_BORDER_INACTIVE;
    }
    view->activate(view, active);
  }
}

static void
migrate_view(struct hikari_view *view, struct hikari_sheet *sheet, bool center)
{
  assert(hikari_view_is_hidden(view));

  view->output = sheet->workspace->output;
  view->sheet = sheet;

  move_to_top(view);

  queue_reset(view, center);
}

void
hikari_view_migrate(struct hikari_view *view,
    struct hikari_sheet *sheet,
    int x,
    int y,
    bool center)
{
  struct hikari_output *output = sheet->workspace->output;
  struct wlr_box *view_geometry = hikari_view_geometry(view);

  hikari_indicator_damage(&hikari_server.indicator, view);
  hikari_view_damage_whole(view);

  // only remove view from lists and do not make it lose focus by calling
  // `hikari_view_hide`.
  hide(view);

  hikari_geometry_constrain_relative(
      &view->geometry, &output->usable_area, x, y);
  hikari_geometry_constrain_relative(view_geometry, &output->usable_area, x, y);

  migrate_view(view, sheet, center);

#ifdef HAVE_XWAYLAND
  if (view->move != NULL) {
    view->move(view, view_geometry->x, view_geometry->y);
  }
#endif

  if (hikari_view_is_hidden(view)) {
    hikari_view_show(view);
  }
}

void
hikari_view_configure(struct hikari_view *view,
    const char *app_id,
    struct hikari_view_config *view_config)
{
  assert(view->id == NULL);

  struct hikari_sheet *sheet;
  struct hikari_output *output;
  struct wlr_box *geometry = &view->geometry;
  int x, y;
  bool invisible, floating, publicview;

  set_app_id(view, app_id);

  if (view_config != NULL) {
    struct hikari_view_properties *properties =
        hikari_view_config_resolve_properties(view_config, view->child);

    sheet = hikari_view_properties_resolve_sheet(properties);
    output = sheet->workspace->output;

    invisible = properties->invisible;
    floating = properties->floating;
    publicview = properties->publicview;

    hikari_view_properties_resolve_position(properties, view, &x, &y);
  } else {
    sheet = hikari_server.workspace->sheet;
    output = sheet->workspace->output;

    invisible = false;
    floating = false;
    publicview = false;

    x = hikari_server.cursor.wlr_cursor->x - output->geometry.x;
    y = hikari_server.cursor.wlr_cursor->y - output->geometry.y;
  }

  view->sheet = sheet;
  view->output = output;

  wl_list_init(&view->workspace_views);
  wl_list_init(&view->visible_server_views);

  if (invisible) {
    hikari_view_set_invisible(view);
  }

  if (floating) {
    hikari_view_set_floating(view);
  }

  if (publicview) {
    hikari_view_set_public(view);
  }

  hikari_geometry_constrain_absolute(geometry, &output->usable_area, x, y);
  hikari_view_refresh_geometry(view, geometry);
}
