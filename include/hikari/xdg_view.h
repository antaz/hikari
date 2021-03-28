#if !defined(HIKARI_XDG_VIEW_H)
#define HIKARI_XDG_VIEW_H

#include <wayland-server-core.h>

#include <wlr/types/wlr_xdg_shell.h>

#include <hikari/view.h>

struct hikari_renderer;

struct hikari_xdg_view {
  struct hikari_view view;

  struct wlr_xdg_surface *surface;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener commit;
  struct wl_listener new_popup;
  struct wl_listener set_title;
  struct wl_listener request_fullscreen;
};

void
hikari_xdg_view_init(struct hikari_xdg_view *xdg_view,
    struct wlr_xdg_surface *xdg_surface,
    struct hikari_workspace *workspace);

struct hikari_xdg_popup {
  struct hikari_view_child view_child;

  struct wlr_xdg_popup *popup;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener new_popup;
};

#endif
