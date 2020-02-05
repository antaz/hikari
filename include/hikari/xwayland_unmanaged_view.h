#if !defined(HIKARI_XWAYLAND_UNMANAGED_VIEW_H)
#define HIKARI_XWAYLAND_UNMANAGED_VIEW_H

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_surface.h>

#include <hikari/view_interface.h>

struct hikari_workspace;

struct hikari_xwayland_unmanaged_view {
  struct hikari_view_interface view_interface;

  struct wlr_xwayland_surface *surface;

  struct hikari_workspace *workspace;
  bool hidden;

  struct wlr_box geometry;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener request_configure;
  struct wl_listener commit;

  struct wl_list unmanaged_server_views;
};

void
hikari_xwayland_unmanaged_view_init(
    struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view,
    struct wlr_xwayland_surface *xwayland_surface,
    struct hikari_workspace *workspace);

#endif
