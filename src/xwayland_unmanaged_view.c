#ifdef HAVE_XWAYLAND
#include <hikari/xwayland_unmanaged_view.h>

#include <wlr/xwayland.h>

#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/server.h>
#include <hikari/workspace.h>

static bool
was_updated(struct wlr_xwayland_surface *surface,
    struct wlr_box *geometry,
    struct hikari_output *output)
{
  return !((output->geometry.x + surface->x == geometry->x) &&
           (output->geometry.y + surface->y == geometry->y) &&
           (surface->width == geometry->width) &&
           (surface->height == geometry->height));
}

static void
commit_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view =
      wl_container_of(listener, xwayland_unmanaged_view, commit);

  struct hikari_output *output = xwayland_unmanaged_view->workspace->output;
  struct wlr_xwayland_surface *surface = xwayland_unmanaged_view->surface;
  struct wlr_box *geometry = &xwayland_unmanaged_view->geometry;

  if (was_updated(surface, geometry, output)) {
    hikari_output_add_damage(output, &xwayland_unmanaged_view->geometry);

    geometry->x = surface->x - output->geometry.x;
    geometry->y = surface->y - output->geometry.y;
    geometry->width = surface->width;
    geometry->height = surface->height;

    hikari_output_add_damage(output, geometry);
  } else if (view->output->enabled) {
    pixman_region32_t damage;
    pixman_region32_init(&damage);
    wlr_surface_get_effective_damage(surface->surface, &damage);
    pixman_region32_translate(&damage, geometry->x, geometry->y);
    wlr_output_damage_add(output->damage, &damage);
    pixman_region32_fini(&damage);
  }
}

static void
map_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view =
      wl_container_of(listener, xwayland_unmanaged_view, map);

#if !defined(NDEBUG)
  printf("UNMANAGED XWAYLAND MAP %p %d %d\n",
      xwayland_unmanaged_view,
      xwayland_unmanaged_view->surface->x,
      xwayland_unmanaged_view->surface->y);
#endif

  struct wlr_box *geometry = &xwayland_unmanaged_view->geometry;
  struct wlr_xwayland_surface *xwayland_surface =
      xwayland_unmanaged_view->surface;
  struct hikari_output *output = xwayland_unmanaged_view->workspace->output;

  xwayland_unmanaged_view->hidden = false;

  geometry->x = xwayland_surface->x - output->geometry.x;
  geometry->y = xwayland_surface->y - output->geometry.y;
  geometry->width = xwayland_surface->width;
  geometry->height = xwayland_surface->height;

  xwayland_unmanaged_view->commit.notify = commit_handler;
  wl_signal_add(&xwayland_surface->surface->events.commit,
      &xwayland_unmanaged_view->commit);

  wl_list_insert(&output->unmanaged_xwayland_views,
      &xwayland_unmanaged_view->unmanaged_server_views);

  hikari_output_add_damage(output, geometry);
}

static void
unmap(struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view)
{
  wl_list_remove(&xwayland_unmanaged_view->commit.link);
  wl_list_remove(&xwayland_unmanaged_view->unmanaged_server_views);

  xwayland_unmanaged_view->hidden = true;

  hikari_output_add_damage(xwayland_unmanaged_view->workspace->output,
      &xwayland_unmanaged_view->geometry);
}

static void
unmap_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view =
      wl_container_of(listener, xwayland_unmanaged_view, unmap);

#if !defined(NDEBUG)
  printf("UNMANAGED XWAYLAND UNMAP %p\n", xwayland_unmanaged_view);
#endif

  unmap(xwayland_unmanaged_view);
}

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view =
      wl_container_of(listener, xwayland_unmanaged_view, destroy);

#if !defined(NDEBUG)
  printf("UNMANAGED XWAYLAND DESTROY %p\n", xwayland_unmanaged_view);
#endif

  if (!xwayland_unmanaged_view->hidden) {
    unmap(xwayland_unmanaged_view);
    hikari_server_cursor_focus();
  }

  wl_list_remove(&xwayland_unmanaged_view->map.link);
  wl_list_remove(&xwayland_unmanaged_view->unmap.link);
  wl_list_remove(&xwayland_unmanaged_view->destroy.link);
  wl_list_remove(&xwayland_unmanaged_view->request_configure.link);

  hikari_free(xwayland_unmanaged_view);
}

static void
request_configure_handler(struct wl_listener *listener, void *data)
{
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view =
      wl_container_of(listener, xwayland_unmanaged_view, request_configure);

  struct wlr_xwayland_surface *surface = xwayland_unmanaged_view->surface;
  struct wlr_xwayland_surface_configure_event *event = data;

  wlr_xwayland_surface_configure(
      surface, event->x, event->y, event->width, event->height);
}

static struct wlr_surface *
surface_at(struct hikari_view_interface *view_interface,
    double lx,
    double ly,
    double *sx,
    double *sy)
{
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view =
      (struct hikari_xwayland_unmanaged_view *)view_interface;

  struct wlr_box *geometry = &xwayland_unmanaged_view->geometry;

  double x = lx - geometry->x;
  double y = ly - geometry->y;

  return wlr_surface_surface_at(
      xwayland_unmanaged_view->surface->surface, x, y, sx, sy);
}

static void
focus(struct hikari_view_interface *view_interface)
{}

void
hikari_xwayland_unmanaged_view_init(
    struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view,
    struct wlr_xwayland_surface *xwayland_surface,
    struct hikari_workspace *workspace)
{
  xwayland_unmanaged_view->workspace = workspace;
  xwayland_unmanaged_view->view_interface.surface_at = surface_at;
  xwayland_unmanaged_view->view_interface.focus = focus;

#if !defined(NDEBUG)
  printf("UNMANAGED XWAYLAND NEW %p\n", xwayland_unmanaged_view);
#endif

  wlr_xwayland_surface_ping(xwayland_surface);

  xwayland_unmanaged_view->surface = xwayland_surface;
  xwayland_unmanaged_view->surface->data =
      (struct hikari_view_interface *)xwayland_unmanaged_view;
  xwayland_unmanaged_view->hidden = true;

  xwayland_unmanaged_view->map.notify = map_handler;
  wl_signal_add(&xwayland_surface->events.map, &xwayland_unmanaged_view->map);

  xwayland_unmanaged_view->unmap.notify = unmap_handler;
  wl_signal_add(
      &xwayland_surface->events.unmap, &xwayland_unmanaged_view->unmap);

  xwayland_unmanaged_view->destroy.notify = destroy_handler;
  wl_signal_add(
      &xwayland_surface->events.destroy, &xwayland_unmanaged_view->destroy);

  xwayland_unmanaged_view->request_configure.notify = request_configure_handler;
  wl_signal_add(&xwayland_surface->events.request_configure,
      &xwayland_unmanaged_view->request_configure);
}
#endif
