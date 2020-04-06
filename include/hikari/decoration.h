#if !defined(HIKARI_DECORATION_H)
#define HIKARI_DECORATION_H

#include <wayland-util.h>

#include <wlr/types/wlr_xdg_decoration_v1.h>

struct hikari_decoration {
  struct wlr_xdg_toplevel_decoration_v1 *decoration;

  struct wl_listener request_mode;
  struct wl_listener destroy;
};

void
hikari_decoration_init(struct hikari_decoration *decoration,
    struct wlr_xdg_toplevel_decoration_v1 *wlr_decoration);

#endif
