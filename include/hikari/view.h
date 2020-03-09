#if !defined(HIKARI_VIEW_H)
#define HIKARI_VIEW_H

#include <assert.h>

#include <wayland-util.h>
#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_surface.h>

#include <hikari/border.h>
#include <hikari/group.h>
#include <hikari/indicator_frame.h>
#include <hikari/maximized_state.h>
#include <hikari/operation.h>
#include <hikari/output.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/tile.h>
#include <hikari/view_interface.h>
#include <hikari/workspace.h>

struct hikari_mark;
struct hikari_render_data;

enum hikari_view_type { HIKARI_XDG_VIEW, HIKARI_XWAYLAND_VIEW };

struct hikari_view;

struct hikari_view_decoration {
  struct wlr_server_decoration *wlr_decoration;
  struct hikari_view *view;
  struct wl_listener mode;
};

struct hikari_view {
  struct hikari_view_interface view_interface;
  enum hikari_view_type type;

  struct hikari_sheet *sheet;
  struct hikari_group *group;
  struct hikari_mark *mark;
  struct hikari_output *output;
  struct wlr_surface *surface;

  bool use_csd;
  unsigned int flags;
  char *title;
  struct hikari_border border;
  struct hikari_indicator_frame indicator_frame;
  struct hikari_tile *tile;

  struct wlr_box geometry;
  struct hikari_maximized_state *maximized_state;

  struct wl_list children;

  struct wl_list output_views;
  struct wl_list workspace_views;
  struct wl_list sheet_views;
  struct wl_list group_views;
  struct wl_list visible_group_views;

  struct hikari_operation pending_operation;

  struct wlr_box *current_geometry;
  struct wlr_box *current_unmaximized_geometry;

  struct hikari_view_decoration decoration;

  uint32_t (*resize)(struct hikari_view *, int, int);
  void (*move)(struct hikari_view *, int, int);
  void (*move_resize)(struct hikari_view *, int, int, int, int);
  void (*activate)(struct hikari_view *, bool);
  void (*quit)(struct hikari_view *);
  void (*constraints)(struct hikari_view *, int *, int *, int *, int *);
};

struct hikari_view_child {
  struct wl_list link;
  struct wlr_surface *surface;
  struct hikari_view *parent;

  struct wl_listener commit;
  struct wl_listener new_subsurface;
};

void
hikari_view_child_init(struct hikari_view_child *view_child,
    struct hikari_view *parent,
    struct wlr_surface *surface);

void
hikari_view_child_fini(struct hikari_view_child *view_child);

struct hikari_view_subsurface {
  struct hikari_view_child view_child;

  struct wlr_subsurface *subsurface;

  struct wl_listener destroy;
};

void
hikari_view_subsurface_init(struct hikari_view_subsurface *view_subsurface,
    struct hikari_view *parent,
    struct wlr_subsurface *subsurface);

void
hikari_view_subsurface_fini(struct hikari_view_subsurface *view_subsurface);

#define FLAG(name, shift)                                                      \
  static const unsigned long hikari_view_##name##_flag = 1UL << shift;         \
                                                                               \
  static inline bool hikari_view_is_##name(struct hikari_view *view)           \
  {                                                                            \
    assert(view != NULL);                                                      \
    return (view->flags & hikari_view_##name##_flag);                          \
  }                                                                            \
                                                                               \
  static inline void hikari_view_set_##name(struct hikari_view *view)          \
  {                                                                            \
    assert(view != NULL);                                                      \
    view->flags |= hikari_view_##name##_flag;                                  \
  }                                                                            \
                                                                               \
  static inline void hikari_view_unset_##name(struct hikari_view *view)        \
  {                                                                            \
    assert(view != NULL);                                                      \
    view->flags &= ~hikari_view_##name##_flag;                                 \
  }

FLAG(hidden, 0UL)
FLAG(iconified, 1UL)
FLAG(floating, 2UL)
#undef FLAG

void
hikari_view_init(struct hikari_view *view,
    enum hikari_view_type type,
    struct hikari_workspace *workspace);

void
hikari_view_fini(struct hikari_view *view);

void
hikari_view_manage(struct hikari_view *view,
    struct hikari_sheet *sheet,
    struct hikari_group *group);

void
hikari_view_set_title(struct hikari_view *view, const char *title);

#define VIEW_ACTION(name) void hikari_view_##name(struct hikari_view *view);

VIEW_ACTION(show)
VIEW_ACTION(hide)
VIEW_ACTION(raise)
VIEW_ACTION(raise_hidden)
VIEW_ACTION(lower)
VIEW_ACTION(toggle_full_maximize)
VIEW_ACTION(toggle_vertical_maximize)
VIEW_ACTION(toggle_horizontal_maximize)
VIEW_ACTION(toggle_floating)
VIEW_ACTION(damage_whole)
VIEW_ACTION(top_left_cursor)
VIEW_ACTION(bottom_right_cursor)
VIEW_ACTION(center_cursor)
VIEW_ACTION(toggle_iconified)
VIEW_ACTION(reset_geometry)
#undef VIEW_ACTION

void
hikari_view_commit_pending_operation(
    struct hikari_view *view, struct wlr_box *geometry);

void
hikari_view_migrate_to_sheet(
    struct hikari_view *view, struct hikari_sheet *sheet);

void
hikari_view_pin_to_sheet(struct hikari_view *view, struct hikari_sheet *sheet);

void
hikari_view_group(struct hikari_view *view, struct hikari_group *group);

void
hikari_view_resize(struct hikari_view *view, int width, int height);

void
hikari_view_move(struct hikari_view *view, int x, int y);

void
hikari_view_move_absolute(struct hikari_view *view, int x, int y);

void
hikari_view_resize_absolute(struct hikari_view *view, int x, int y);

void
hikari_view_assign_sheet(struct hikari_view *view, struct hikari_sheet *sheet);

void
hikari_view_tile(struct hikari_view *view, struct wlr_box *geometry);

void
hikari_view_exchange(struct hikari_view *from, struct hikari_view *to);

void
hikari_view_damage_surface(
    struct hikari_view *view, struct wlr_surface *surface, bool whole);

void
hikari_view_refresh_geometry(
    struct hikari_view *view, struct wlr_box *geometry);

void
hikari_view_activate(struct hikari_view *view, bool active);

static inline bool
hikari_view_is_dirty(struct hikari_view *view)
{
  assert(view != NULL);
  return view->pending_operation.dirty;
}

static inline void
hikari_view_set_dirty(struct hikari_view *view)
{
  assert(view != NULL);
  view->pending_operation.dirty = true;
}

static inline void
hikari_view_unset_dirty(struct hikari_view *view)
{
  assert(view != NULL);
  view->pending_operation.dirty = false;
}

static inline bool
hikari_view_was_updated(struct hikari_view *view, uint32_t serial)
{
  assert(view != NULL);
  return hikari_view_is_dirty(view) && serial >= view->pending_operation.serial;
}

static inline void
hikari_view_quit(struct hikari_view *view)
{
  assert(view != NULL);
  if (view->quit) {
    view->quit(view);
  }
}

static inline bool
hikari_view_is_fully_maximized(struct hikari_view *view)
{
  assert(view != NULL);
  return view->maximized_state != NULL &&
         view->maximized_state->maximization ==
             HIKARI_MAXIMIZATION_FULLY_MAXIMIZED;
}

static inline void
hikari_view_for_each_surface(
    struct hikari_view *view, wlr_surface_iterator_func_t func, void *data)
{
  assert(view != NULL);
  assert(view->surface != NULL);

  wlr_surface_for_each_surface(view->surface, func, data);
}

static inline struct wlr_box *
hikari_view_geometry(struct hikari_view *view)
{
  assert(view != NULL);
  return view->current_geometry;
}

static inline struct wlr_box *
hikari_view_border_geometry(struct hikari_view *view)
{
  assert(view != NULL);
  return &view->border.geometry;
}

static inline bool
hikari_view_has_focus(struct hikari_view *view)
{
  assert(view != NULL);
  return hikari_server.workspace->focus_view == view;
}

static inline bool
hikari_view_wants_border(struct hikari_view *view)
{
  assert(view != NULL);
  return view->border.state != HIKARI_BORDER_NONE;
}

static inline bool
hikari_view_is_tiled(struct hikari_view *view)
{
  assert(view != NULL);
  return view->tile != NULL;
}

static inline bool
hikari_view_is_tileable(struct hikari_view *view)
{
  return !hikari_view_is_floating(view) && !hikari_view_is_iconified(view) &&
         !hikari_view_is_dirty(view);
}

static inline bool
hikari_view_is_mapped(struct hikari_view *view)
{
  assert(view != NULL);
  return view->surface != NULL;
}

static inline bool
hikari_view_is_maximized(struct hikari_view *view)
{
  return view->maximized_state != NULL;
}

static inline bool
hikari_view_is_unmanaged(struct hikari_view *view)
{
  return view->sheet == NULL;
}

static inline bool
hikari_view_is_tiling(struct hikari_view *view)
{
  return hikari_view_is_dirty(view) &&
         view->pending_operation.type == HIKARI_OPERATION_TYPE_TILE;
}

#endif
