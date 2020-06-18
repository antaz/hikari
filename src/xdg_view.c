#include <hikari/xdg_view.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/edges.h>

#include <hikari/configuration.h>
#include <hikari/geometry.h>
#include <hikari/mark.h>
#include <hikari/output.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/view.h>
#include <hikari/view_autoconf.h>
#include <hikari/workspace.h>

static void
new_popup_handler(struct wl_listener *listener, void *data);

static void
new_subsurface_handler(struct wl_listener *listener, void *data);

static void
request_fullscreen_handler(struct wl_listener *listener, void *data);

static void
set_title_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xdg_view *xdg_view =
      wl_container_of(listener, xdg_view, set_title);

  hikari_view_set_title(
      (struct hikari_view *)xdg_view, xdg_view->surface->toplevel->title);
}

static void
commit_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xdg_view *xdg_view =
      wl_container_of(listener, xdg_view, commit);
  struct hikari_view *view = (struct hikari_view *)xdg_view;

  assert(view->surface != NULL);

  uint32_t serial = xdg_view->surface->configure_serial;

  if (hikari_view_was_updated(view, serial)) {
    struct wlr_box new_geometry;
    wlr_xdg_surface_get_geometry(xdg_view->surface, &new_geometry);

    switch (view->pending_operation.type) {
      case HIKARI_OPERATION_TYPE_TILE:
      case HIKARI_OPERATION_TYPE_FULL_MAXIMIZE:
      case HIKARI_OPERATION_TYPE_VERTICAL_MAXIMIZE:
      case HIKARI_OPERATION_TYPE_HORIZONTAL_MAXIMIZE:
        wlr_xdg_toplevel_set_tiled(xdg_view->surface,
            WLR_EDGE_LEFT | WLR_EDGE_RIGHT | WLR_EDGE_TOP | WLR_EDGE_BOTTOM);
        break;

      case HIKARI_OPERATION_TYPE_RESET:
      case HIKARI_OPERATION_TYPE_UNMAXIMIZE:
        wlr_xdg_toplevel_set_tiled(xdg_view->surface, WLR_EDGE_NONE);
        break;

      case HIKARI_OPERATION_TYPE_RESIZE:
        break;
    }
    hikari_view_commit_pending_operation(view, &new_geometry);
  } else {
    struct wlr_box *geometry = hikari_view_geometry(view);

    struct wlr_box new_geometry;
    wlr_xdg_surface_get_geometry(xdg_view->surface, &new_geometry);

    if (new_geometry.width != geometry->width ||
        new_geometry.height != geometry->height) {
      if (!hikari_view_is_hidden(view)) {
        hikari_view_damage_whole(view);
      }

      geometry->width = new_geometry.width;
      geometry->height = new_geometry.height;

      hikari_view_refresh_geometry(view, geometry);

      if (!hikari_view_is_hidden(view)) {
        hikari_view_damage_whole(view);
      }
    } else if (!hikari_view_is_hidden(view) && view->output->enabled) {
      pixman_region32_t damage;
      pixman_region32_init(&damage);
      wlr_surface_get_effective_damage(xdg_view->surface->surface, &damage);
      pixman_region32_translate(&damage, geometry->x, geometry->y);
      wlr_output_damage_add(view->output->damage, &damage);
      pixman_region32_fini(&damage);
    }
  }
}

static void
first_map(struct hikari_xdg_view *xdg_view, bool *focus)
{
  assert(xdg_view->surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);

  struct hikari_view *view = (struct hikari_view *)xdg_view;
  struct wlr_box *geometry = &xdg_view->view.geometry;

  wlr_xdg_surface_get_geometry(xdg_view->surface, geometry);
  hikari_view_refresh_geometry(view, geometry);

  const char *app_id = xdg_view->surface->toplevel->app_id;

#if !defined(NDEBUG)
  printf("APP ID %s\n", app_id);
#endif

  struct hikari_view_autoconf *view_autoconf =
      hikari_configuration_resolve_view_autoconf(hikari_configuration, app_id);

  hikari_view_set_title(view, xdg_view->surface->toplevel->title);
  hikari_view_configure(view, app_id, view_autoconf);
}

static struct wlr_surface *
surface_at(struct hikari_view_interface *view_interface,
    double ox,
    double oy,
    double *sx,
    double *sy)
{
  struct hikari_xdg_view *xdg_view = (struct hikari_xdg_view *)view_interface;

  struct hikari_view *view = (struct hikari_view *)view_interface;

  struct wlr_box *geometry = hikari_view_geometry(view);

  double x = ox - geometry->x;
  double y = oy - geometry->y;

  return wlr_xdg_surface_surface_at(xdg_view->surface, x, y, sx, sy);
}

static void
map(struct hikari_view *view, bool focus)
{
#if !defined(NDEBUG)
  printf("XDG MAP %p\n", view);
#endif

  struct hikari_xdg_view *xdg_view = (struct hikari_xdg_view *)view;
  struct wlr_xdg_surface *xdg_surface = xdg_view->surface;

  xdg_view->set_title.notify = set_title_handler;
  wl_signal_add(
      &xdg_view->surface->toplevel->events.set_title, &xdg_view->set_title);

  xdg_view->request_fullscreen.notify = request_fullscreen_handler;
  wl_signal_add(&xdg_surface->toplevel->events.request_fullscreen,
      &xdg_view->request_fullscreen);

  xdg_view->new_popup.notify = new_popup_handler;
  wl_signal_add(&xdg_surface->events.new_popup, &xdg_view->new_popup);

  xdg_view->new_subsurface.notify = new_subsurface_handler;
  wl_signal_add(
      &xdg_surface->surface->events.new_subsurface, &xdg_view->new_subsurface);

  xdg_view->commit.notify = commit_handler;
  wl_signal_add(&xdg_view->surface->surface->events.commit, &xdg_view->commit);

  hikari_view_map(view, xdg_surface->surface);
}

static void
map_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xdg_view *xdg_view = wl_container_of(listener, xdg_view, map);

  struct hikari_view *view = (struct hikari_view *)xdg_view;
  bool focus = false;

  if (hikari_view_is_unmanaged(view)) {
    first_map(xdg_view, &focus);
  }

  map(view, focus);
}

static void
unmap(struct hikari_view *view)
{
#if !defined(NDEBUG)
  printf("XDG UNMAP %p\n", view);
#endif

  if (!hikari_view_is_hidden(view)) {
    hikari_view_hide(view);
    hikari_server_cursor_focus();
  }

  struct hikari_xdg_view *xdg_view = (struct hikari_xdg_view *)view;

  assert(xdg_view->surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);

  hikari_view_unmap(view);

  wl_list_remove(&xdg_view->set_title.link);
  wl_list_remove(&xdg_view->request_fullscreen.link);
  wl_list_remove(&xdg_view->new_popup.link);
  wl_list_remove(&xdg_view->new_subsurface.link);
  wl_list_remove(&xdg_view->commit.link);
}

static void
unmap_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xdg_view *xdg_view = wl_container_of(listener, xdg_view, unmap);

  unmap((struct hikari_view *)xdg_view);
}

static void
activate(struct hikari_view *view, bool active)
{
  struct hikari_xdg_view *xdg_view = (struct hikari_xdg_view *)view;

  if (xdg_view->surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    wlr_xdg_toplevel_set_activated(xdg_view->surface, active);

    hikari_view_damage_whole(view);
  }
}

static uint32_t
resize(struct hikari_view *view, int width, int height)
{
  struct hikari_xdg_view *xdg_view = (struct hikari_xdg_view *)view;

  if (xdg_view->surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return wlr_xdg_toplevel_set_size(xdg_view->surface, width, height);
  }

  return 0;
}

static void
quit(struct hikari_view *view)
{
  struct hikari_xdg_view *xdg_view = (struct hikari_xdg_view *)view;

  wlr_xdg_toplevel_send_close(xdg_view->surface);
}

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xdg_view *xdg_view =
      wl_container_of(listener, xdg_view, destroy);

  struct hikari_view *view = (struct hikari_view *)xdg_view;

  if (hikari_view_is_mapped(view)) {
    unmap(view);
  }

  wl_list_remove(&xdg_view->map.link);
  wl_list_remove(&xdg_view->unmap.link);
  wl_list_remove(&xdg_view->destroy.link);

  hikari_view_fini(view);
  hikari_free(xdg_view);
}

static void
focus(struct hikari_view_interface *view_interface)
{
  struct hikari_view *view = (struct hikari_view *)view_interface;

  hikari_workspace_focus_view(view->sheet->workspace, view);
}

static void
for_each_surface(struct hikari_view_interface *view_interface,
    void (*func)(struct wlr_surface *, int, int, void *),
    void *data)
{
  struct hikari_xdg_view *xdg_view = (struct hikari_xdg_view *)view_interface;

  wlr_xdg_surface_for_each_surface(xdg_view->surface, func, data);
}

static void
destroy_popup_handler(struct wl_listener *listener, void *data)
{
#if !defined(NDEBUG)
  printf("DESTROY POPUP\n");
#endif
  struct hikari_xdg_popup *popup = wl_container_of(listener, popup, destroy);

  hikari_view_child_fini(&popup->view_child);

  wl_list_remove(&popup->destroy.link);
  wl_list_remove(&popup->unmap.link);
  wl_list_remove(&popup->map.link);
  wl_list_remove(&popup->new_popup.link);

  hikari_free(popup);
}

static void
xdg_popup_create(struct wlr_xdg_popup *wlr_popup, struct hikari_view *parent);

static void
new_popup_popup_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xdg_popup *xdg_popup =
      wl_container_of(listener, xdg_popup, new_popup);

  struct wlr_xdg_popup *wlr_popup = data;

  xdg_popup_create(wlr_popup, xdg_popup->view_child.parent);
}

static void
new_popup_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xdg_view *xdg_view =
      wl_container_of(listener, xdg_view, new_popup);

  struct wlr_xdg_popup *wlr_popup = data;

  xdg_popup_create(wlr_popup, &xdg_view->view);
}

static void
popup_map(struct wl_listener *listener, void *data)
{
#if !defined(NDEBUG)
  printf("POPUP MAP\n");
#endif

  struct hikari_xdg_popup *xdg_popup =
      wl_container_of(listener, xdg_popup, map);

  struct hikari_view *parent = xdg_popup->view_child.parent;

  hikari_view_damage_surface(parent, xdg_popup->view_child.surface, true);
}

static void
popup_unmap(struct wl_listener *listener, void *data)
{
#if !defined(NDEBUG)
  printf("POPUP UNMAP\n");
#endif

  struct hikari_xdg_popup *xdg_popup =
      wl_container_of(listener, xdg_popup, unmap);

  struct hikari_view *parent = xdg_popup->view_child.parent;

  hikari_view_damage_surface(parent, xdg_popup->view_child.surface, true);
}

static void
popup_unconstrain(struct hikari_xdg_popup *popup)
{
  struct hikari_view *view = popup->view_child.parent;
  struct wlr_xdg_popup *wlr_popup = popup->popup;

  struct hikari_output *output = view->output;

  struct wlr_box *geometry = hikari_view_geometry(view);

  struct wlr_box output_toplevel_sx_box = {
    .x = -geometry->x,
    .y = -geometry->y,
    .width = output->geometry.width,
    .height = output->geometry.height,
  };

  wlr_xdg_popup_unconstrain_from_box(wlr_popup, &output_toplevel_sx_box);
}

static void
xdg_popup_create(struct wlr_xdg_popup *wlr_popup, struct hikari_view *parent)
{
  struct hikari_xdg_popup *popup =
      hikari_malloc(sizeof(struct hikari_xdg_popup));

#if !defined(NDEBUG)
  printf("CREATE POPUP\n");
#endif

  popup->view_child.parent = parent;
  popup->popup = wlr_popup;

  wlr_popup->base->surface->data = parent;

  popup->destroy.notify = destroy_popup_handler;
  wl_signal_add(&wlr_popup->base->events.destroy, &popup->destroy);

  popup->new_popup.notify = new_popup_popup_handler;
  wl_signal_add(&wlr_popup->base->events.new_popup, &popup->new_popup);

  popup->map.notify = popup_map;
  wl_signal_add(&wlr_popup->base->events.map, &popup->map);

  popup->unmap.notify = popup_unmap;
  wl_signal_add(&wlr_popup->base->events.unmap, &popup->unmap);

  hikari_view_child_init(
      (struct hikari_view_child *)popup, parent, wlr_popup->base->surface);

  popup_unconstrain(popup);
}

static void
new_subsurface_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xdg_view *xdg_view =
      wl_container_of(listener, xdg_view, new_subsurface);

  struct wlr_subsurface *subsurface = data;

  struct hikari_view_subsurface *view_subsurface =
      hikari_malloc(sizeof(struct hikari_view_subsurface));

  hikari_view_subsurface_init(view_subsurface, &xdg_view->view, subsurface);

#if !defined(NDEBUG)
  printf("SUBSURFACE\n");
#endif
}

static void
request_fullscreen_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xdg_view *xdg_view =
      wl_container_of(listener, xdg_view, request_fullscreen);

  struct wlr_xdg_toplevel_set_fullscreen_event *event = data;

  wlr_xdg_toplevel_set_fullscreen(xdg_view->surface, event->fullscreen);
}

static void
constraints(struct hikari_view *view,
    int *min_width,
    int *min_height,
    int *max_width,
    int *max_height)
{
  struct hikari_xdg_view *xdg_view = (struct hikari_xdg_view *)view;
  struct wlr_xdg_toplevel_state *state = &xdg_view->surface->toplevel->current;

  *min_width = state->min_width > 0 ? state->min_width : 0;
  *min_height = state->min_height > 0 ? state->min_height : 0;
  *max_width =
      state->max_width > 0 ? state->max_width : view->output->geometry.width;
  *max_height =
      state->max_height > 0 ? state->max_height : view->output->geometry.height;
}

void
hikari_xdg_view_init(struct hikari_xdg_view *xdg_view,
    struct wlr_xdg_surface *xdg_surface,
    struct hikari_workspace *workspace)
{
  hikari_view_init(&xdg_view->view, HIKARI_XDG_VIEW, workspace);

#if !defined(NDEBUG)
  printf("NEW XDG %p\n", xdg_view);
#endif

  xdg_view->view.view_interface.surface_at = surface_at;

  wlr_xdg_surface_ping(xdg_surface);

  xdg_view->surface = xdg_surface;
  xdg_view->surface->data = xdg_view;

  xdg_view->map.notify = map_handler;
  wl_signal_add(&xdg_surface->events.map, &xdg_view->map);

  xdg_view->unmap.notify = unmap_handler;
  wl_signal_add(&xdg_surface->events.unmap, &xdg_view->unmap);

  xdg_view->destroy.notify = destroy_handler;
  wl_signal_add(&xdg_surface->events.destroy, &xdg_view->destroy);

  assert(xdg_view->surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);

  xdg_view->view.view_interface.focus = focus;
  xdg_view->view.view_interface.for_each_surface = for_each_surface;
  xdg_view->view.activate = activate;
  xdg_view->view.resize = resize;
  xdg_view->view.move_resize = NULL;
  xdg_view->view.quit = quit;
  xdg_view->view.constraints = constraints;
  xdg_view->view.move = NULL;
}
