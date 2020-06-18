#if !defined(HIKARI_XWAYLAND_VIEW_H)
#define HIKARI_XWAYLAND_VIEW_H

#include <wayland-server-core.h>

#include <hikari/view.h>

struct hikari_renderer;

struct hikari_xwayland_view {
  struct hikari_view view;

  struct wlr_xwayland_surface *surface;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener request_configure;
  struct wl_listener commit;
  struct wl_listener set_title;
};

void
hikari_xwayland_view_init(struct hikari_xwayland_view *xwayland_view,
    struct wlr_xwayland_surface *xwayland_surface,
    struct hikari_workspace *workspace);

#endif
