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
move_resize(struct hikari_view *view, int x, int y, int width, int height)
{
  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;

  struct hikari_output *output = view->output;

  wlr_xwayland_surface_configure(xwayland_view->surface,
      output->geometry.x + x,
      output->geometry.y + y,
      width,
      height);
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

static void
commit_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_view *xwayland_view =
      wl_container_of(listener, xwayland_view, commit);

  struct hikari_view *view = (struct hikari_view *)xwayland_view;
  struct wlr_box *geometry = hikari_view_geometry(view);

  if (hikari_view_is_dirty(view)) {
    hikari_view_commit_pending_operation(
        view, &view->pending_operation.geometry);
  } else {
    struct wlr_xwayland_surface *surface = xwayland_view->surface;
    struct hikari_output *output = view->output;

    // Xwayland surfaces use "output layout coordinates", hikari's coordinates
    // are relative to output - convert
    const int surface_x_in_hikari = surface->x - output->geometry.x;
    const int surface_y_in_hikari = surface->y - output->geometry.y;

    if (surface->width != geometry->width ||
        surface->height != geometry->height ||
        surface_x_in_hikari != geometry->x ||
        surface_y_in_hikari != geometry->y) {
      bool visible = !hikari_view_is_hidden(view);

      if (visible) {
        hikari_indicator_damage(&hikari_server.indicator, view);
        hikari_view_damage_whole(view);
      }

      geometry->x = surface_x_in_hikari;
      geometry->y = surface_y_in_hikari;
      geometry->width = surface->width;
      geometry->height = surface->height;

      hikari_view_refresh_geometry(view, geometry);

      if (visible) {
        hikari_view_damage_whole(view);
      } else if (output->enabled) {
        hikari_output_schedule_frame(output);
      }
    } else if (output->enabled) {
      bool visible = !hikari_view_is_hidden(view);

      if (visible) {
        hikari_output_add_effective_surface_damage(
            output, surface->surface, geometry->x, geometry->y);
      } else {
        hikari_output_schedule_frame(output);
      }
    }
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

static const char *
get_class(struct wlr_xwayland_surface *surface)
{
  const char *app_id = surface->class;

  if (app_id == NULL) {
    return "";
  } else {
    return app_id;
  }
}

static void
first_map(struct hikari_xwayland_view *xwayland_view, bool *focus)
{
  struct hikari_view *view = (struct hikari_view *)xwayland_view;
  struct wlr_xwayland_surface *xwayland_surface = xwayland_view->surface;
  struct wlr_box *geometry = &view->geometry;

  view->border.state = HIKARI_BORDER_INACTIVE;

  geometry->width = xwayland_surface->width;
  geometry->height = xwayland_surface->height;
  hikari_view_refresh_geometry(view, geometry);

  const char *app_id = get_class(xwayland_surface);

  struct hikari_view_config *view_config =
      hikari_configuration_resolve_view_config(hikari_configuration, app_id);

  hikari_view_set_title(view, xwayland_surface->title);
  hikari_view_configure(view, app_id, view_config);

  struct hikari_output *output = view->output;

  wlr_xwayland_surface_configure(xwayland_view->surface,
      output->geometry.x + view->geometry.x,
      output->geometry.y + view->geometry.y,
      geometry->width,
      geometry->height);
}

static void
map(struct hikari_view *view, bool focus)
{
#if !defined(NDEBUG)
  printf("XWAYLAND MAP %p\n", view);
#endif

  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;

  xwayland_view->commit.notify = commit_handler;
  wl_signal_add(
      &xwayland_view->surface->surface->events.commit, &xwayland_view->commit);

  hikari_view_map(view, xwayland_view->surface->surface);
}

static void
map_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_view *xwayland_view =
      wl_container_of(listener, xwayland_view, map);

  struct hikari_view *view = (struct hikari_view *)xwayland_view;
  bool focus = false;

  if (hikari_view_is_unmanaged(view)) {
    first_map(xwayland_view, &focus);
  }

  map(view, focus);
}

static void
unmap(struct hikari_view *view)
{
#if !defined(NDEBUG)
  printf("XWAYLAND UNMAP %p\n", view);
#endif

  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;

  hikari_view_unmap(view);

  wl_list_remove(&xwayland_view->commit.link);
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

  struct hikari_output *output = sheet != NULL
                                     ? xwayland_view->view.output
                                     : hikari_server.workspace->output;
  struct wlr_box *usable_area = &output->usable_area;

  hikari_geometry_constrain_absolute(&geometry,
      usable_area,
      event->x - output->geometry.x,
      event->y - output->geometry.y);

  wlr_xwayland_surface_configure(xwayland_surface,
      geometry.x + output->geometry.x,
      geometry.y + output->geometry.y,
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
surface_at(
    struct hikari_node *node, double ox, double oy, double *sx, double *sy)
{
  struct hikari_view *view = (struct hikari_view *)node;

  struct wlr_box *geometry = hikari_view_geometry(view);

  double x = ox - geometry->x;
  double y = oy - geometry->y;

  return wlr_surface_surface_at(view->surface, x, y, sx, sy);
}

static void
focus(struct hikari_node *node)
{
  struct hikari_view *view = (struct hikari_view *)node;

  hikari_workspace_focus_view(view->sheet->workspace, view);
}

static void
for_each_surface(struct hikari_node *node,
    void (*func)(struct wlr_surface *, int, int, void *),
    void *data)
{
  struct hikari_view *view = (struct hikari_view *)node;
  struct wlr_surface *surface = view->surface;

  if (surface != NULL) {
    wlr_surface_for_each_surface(surface, func, data);
  }
}

static void
constraints(struct hikari_view *view,
    int *min_width,
    int *min_height,
    int *max_width,
    int *max_height)
{
  struct hikari_xwayland_view *xwayland_view =
      (struct hikari_xwayland_view *)view;
  struct hikari_output *output = view->output;
  struct wlr_xwayland_surface *surface = xwayland_view->surface;

  struct wlr_xwayland_surface_size_hints *size_hints = surface->size_hints;

  if (size_hints != NULL) {
    *min_width = size_hints->min_width > 0 ? size_hints->min_width : 0;
    *max_width = size_hints->max_width > 0 ? size_hints->max_width
                                           : output->geometry.width;
    *min_height = size_hints->min_height > 0 ? size_hints->min_height : 0;
    *max_height = size_hints->max_height > 0 ? size_hints->max_height
                                             : output->geometry.height;
  } else {
    *min_width = 0;
    *max_width = output->geometry.width;

    *min_height = 0;
    *max_height = output->geometry.height;
  }
}

void
hikari_xwayland_view_init(struct hikari_xwayland_view *xwayland_view,
    struct wlr_xwayland_surface *xwayland_surface,
    struct hikari_workspace *workspace)
{
  struct hikari_view *view = &xwayland_view->view;

  hikari_view_init(view, false, workspace);

  view->node.surface_at = surface_at;
  view->node.focus = focus;
  view->node.for_each_surface = for_each_surface;

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

  view->activate = activate;
  view->resize = resize;
  view->move = move;
  view->move_resize = move_resize;
  view->quit = quit;
  view->constraints = constraints;
}
#endif
