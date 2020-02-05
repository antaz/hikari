#ifdef HAVE_XWAYLAND
#include <hikari/xwayland_view.h>

#include <assert.h>
#include <stdlib.h>

#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/xwayland.h>

#include <hikari/configuration.h>
#include <hikari/geometry.h>
#include <hikari/output.h>
#include <hikari/render_data.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

static uint32_t
resize(struct hikari_view *view, int width, int height)
{
  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;

  struct wlr_box *geometry = hikari_view_geometry(view);
  struct hikari_output *output = view->output;

  wlr_xwayland_surface_configure(xwayland_view->surface,
      output->geometry.x + geometry->x,
      output->geometry.y + geometry->y,
      width,
      height);

  return 0;
}

static void
move(struct hikari_view *view, int x, int y)
{
  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;

  struct wlr_box *geometry = hikari_view_geometry(view);
  struct hikari_output *output = view->output;

  wlr_xwayland_surface_configure(xwayland_view->surface,
      output->geometry.x + x,
      output->geometry.y + y,
      geometry->width,
      geometry->height);
}

static bool
was_updated(struct hikari_xwayland_view *xwayland_view)
{
  struct hikari_view *view = (struct hikari_view *)xwayland_view;

  struct wlr_xwayland_surface *surface = xwayland_view->surface;
  struct wlr_box *pending_geometry = &view->pending_operation.geometry;

  return hikari_view_is_dirty(view) &&
         (surface->width == pending_geometry->width ||
             surface->height == pending_geometry->height);
}

static bool
was_moved(struct wlr_xwayland_surface *surface,
    struct wlr_box *geometry,
    struct hikari_output *output)
{
  return !((output->geometry.x + surface->x == geometry->x) &&
           (output->geometry.y + surface->y == geometry->y));
}

static void
commit_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_view *xwayland_view =
      wl_container_of(listener, xwayland_view, commit);

  struct hikari_view *view = (struct hikari_view *)xwayland_view;
  struct wlr_box *geometry = hikari_view_geometry(view);
  struct hikari_output *output = view->output;
  struct wlr_xwayland_surface *surface = xwayland_view->surface;

  if (was_updated(xwayland_view)) {
    hikari_view_commit_pending_operation(view, geometry);

    geometry = hikari_view_geometry(view);

    wlr_xwayland_surface_configure(surface,
        output->geometry.x + geometry->x,
        output->geometry.y + geometry->y,
        geometry->width,
        geometry->height);
  } else if (was_moved(surface, geometry, output)) {
    hikari_view_damage_whole(view);
    hikari_indicator_damage(&hikari_server.indicator, view);

    geometry->x = surface->x - output->geometry.x;
    geometry->y = surface->y - output->geometry.y;

    hikari_view_damage_whole(view);
  } else {
    pixman_region32_t damage;
    pixman_region32_init(&damage);
    wlr_surface_get_effective_damage(surface->surface, &damage);
    pixman_region32_translate(&damage, geometry->x, geometry->y);
    wlr_output_damage_add(output->damage, &damage);
    pixman_region32_fini(&damage);
    wlr_output_schedule_frame(output->output);
  }
}

static void
set_title_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_view *xwayland_view =
      wl_container_of(listener, xwayland_view, set_title);

  struct hikari_view *view = (struct hikari_view *)xwayland_view;

  hikari_view_set_title(view, xwayland_view->surface->title);
}

static void
first_map(struct hikari_xwayland_view *xwayland_view)
{
  struct hikari_view *view = (struct hikari_view *)xwayland_view;
  struct hikari_output *output = hikari_server.workspace->output;

  view->border = HIKARI_BORDER_INACTIVE;

  struct hikari_sheet *sheet;
  struct hikari_group *group;
  int x;
  int y;
  bool focus;

  hikari_configuration_resolve_view_autoconf(&hikari_configuration,
      xwayland_view->surface->class,
      view,
      &sheet,
      &group,
      &x,
      &y,
      &focus);

  if (x == -1) {
    x = hikari_server.cursor->x - output->geometry.x;
  }

  if (y == -1) {
    y = hikari_server.cursor->y - output->geometry.y;
  }

  view->geometry.width = xwayland_view->surface->width;
  view->geometry.height = xwayland_view->surface->height;

  xwayland_view->commit.notify = commit_handler;
  wl_signal_add(
      &xwayland_view->surface->surface->events.commit, &xwayland_view->commit);

  hikari_view_set_title(view, xwayland_view->surface->title);
  hikari_view_manage(view, sheet, group);

  int screen_width, screen_height;
  wlr_output_effective_resolution(
      output->output, &screen_width, &screen_height);

  hikari_geometry_constrain_position(
      &view->geometry, screen_width, screen_height, x, y);

  wlr_xwayland_surface_configure(xwayland_view->surface,
      output->geometry.x + view->geometry.x,
      output->geometry.y + view->geometry.y,
      view->geometry.width,
      view->geometry.height);
}

static void
map(struct hikari_view *view)
{
#if !defined(NDEBUG)
  printf("XWAYLAND MAP %p\n", view);
#endif

  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;

  view->surface = xwayland_view->surface->surface;
  view->surface->data = (struct hikari_view_interface *)view;

  /* if (view->sheet == view->sheet->workspace->sheet || */
  /*     view->sheet == &view->sheet->workspace->sheets[0]) { */
  hikari_view_show(view);
  hikari_server_cursor_focus();
  /* } */
}

static void
map_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_view *xwayland_view =
      wl_container_of(listener, xwayland_view, map);

  struct hikari_view *view = (struct hikari_view *)xwayland_view;

  if (view->sheet == NULL) {
    first_map(xwayland_view);
  }

  map(view);
}

static void
unmap(struct hikari_view *view)
{
#if !defined(NDEBUG)
  printf("XWAYLAND UNMAP %p\n", view);
#endif

  if (!hikari_view_is_hidden(view)) {
    hikari_view_hide(view);
    hikari_server_cursor_focus();
  }

  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;

  wl_list_remove(&xwayland_view->commit.link);

  view->surface = NULL;
}

static void
unmap_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_view *xwayland_view =
      wl_container_of(listener, xwayland_view, unmap);

  unmap((struct hikari_view *)xwayland_view);
}

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_view *xwayland_view =
      wl_container_of(listener, xwayland_view, destroy);

#if !defined(NDEBUG)
  printf("XWAYLAND DESTROY %p\n", xwayland_view);
#endif

  struct hikari_view *view = (struct hikari_view *)xwayland_view;

  if (hikari_view_is_mapped(view)) {
    unmap(view);
  }

  hikari_view_fini(&xwayland_view->view);

  wl_list_remove(&xwayland_view->map.link);
  wl_list_remove(&xwayland_view->unmap.link);
  wl_list_remove(&xwayland_view->destroy.link);
  wl_list_remove(&xwayland_view->request_configure.link);
  wl_list_remove(&xwayland_view->set_title.link);

  hikari_free(xwayland_view);
}

static void
request_configure_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_view *xwayland_view =
      wl_container_of(listener, xwayland_view, request_configure);

  struct wlr_xwayland_surface *xwayland_surface = xwayland_view->surface;
  struct wlr_xwayland_surface_configure_event *event = data;

#if !defined(NDEBUG)
  printf("XWAYLAND CONFIGURE %p %d %d\n", xwayland_view, event->x, event->y);
#endif

  struct wlr_box geometry = {
    .x = event->x, .y = event->y, .width = event->width, .height = event->height
  };

  struct hikari_sheet *sheet = xwayland_view->view.sheet;

  int screen_width, screen_height;
  if (sheet != NULL) {
    wlr_output_effective_resolution(
        xwayland_view->view.output->output, &screen_width, &screen_height);
  } else {
    wlr_output_effective_resolution(
        hikari_server.workspace->output->output, &screen_width, &screen_height);
  }

  hikari_geometry_constrain_position(
      &geometry, screen_width, screen_height, event->x, event->y);

  wlr_xwayland_surface_configure(xwayland_surface,
      geometry.x,
      geometry.y,
      geometry.width,
      geometry.height);
}

static void
activate(struct hikari_view *view, bool active)
{
  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;

  struct wlr_xwayland_surface *xwayland_surface = xwayland_view->surface;

  wlr_xwayland_surface_activate(xwayland_surface, active);
  wlr_xwayland_set_seat(hikari_server.xwayland, hikari_server.seat);
}

static void
quit(struct hikari_view *view)
{
  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;

  wlr_xwayland_surface_close(xwayland_view->surface);
}

static struct wlr_surface *
surface_at(struct hikari_view_interface *view_interface,
    double ox,
    double oy,
    double *sx,
    double *sy)
{
  struct hikari_view *view = (struct hikari_view *)view_interface;

  struct wlr_box *geometry = hikari_view_geometry(view);

  double x = ox - geometry->x;
  double y = oy - geometry->y;

  return wlr_surface_surface_at(view->surface, x, y, sx, sy);
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
  struct hikari_view *view = (struct hikari_view *)view_interface;

  wlr_surface_for_each_surface(view->surface, func, data);
}

static void
hide(struct hikari_view *view)
{}

static void
show(struct hikari_view *view)
{}

void
hikari_xwayland_view_init(struct hikari_xwayland_view *xwayland_view,
    struct wlr_xwayland_surface *xwayland_surface,
    struct hikari_workspace *workspace)
{
  hikari_view_init(&xwayland_view->view, HIKARI_XWAYLAND_VIEW, workspace);

  xwayland_view->view.view_interface.surface_at = surface_at;
  xwayland_view->view.view_interface.focus = focus;
  xwayland_view->view.view_interface.for_each_surface = for_each_surface;

  wlr_xwayland_surface_ping(xwayland_surface);

#if !defined(NDEBUG)
  printf("XWAYLAND NEW %p\n", xwayland_view);
#endif

  xwayland_view->surface = xwayland_surface;

  xwayland_view->map.notify = map_handler;
  wl_signal_add(&xwayland_surface->events.map, &xwayland_view->map);

  xwayland_view->unmap.notify = unmap_handler;
  wl_signal_add(&xwayland_surface->events.unmap, &xwayland_view->unmap);

  xwayland_view->destroy.notify = destroy_handler;
  wl_signal_add(&xwayland_surface->events.destroy, &xwayland_view->destroy);

  xwayland_view->request_configure.notify = request_configure_handler;
  wl_signal_add(&xwayland_surface->events.request_configure,
      &xwayland_view->request_configure);

  xwayland_view->set_title.notify = set_title_handler;
  wl_signal_add(&xwayland_surface->events.set_title, &xwayland_view->set_title);

  xwayland_view->view.activate = activate;
  xwayland_view->view.resize = resize;
  xwayland_view->view.move = move;
  xwayland_view->view.quit = quit;
  xwayland_view->view.hide = hide;
  xwayland_view->view.show = show;
}
#endif
