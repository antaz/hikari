#if !defined(HIKARI_VIEW_INTERFACE_H)
#define HIKARI_VIEW_INTERFACE_H

#include <assert.h>

#include <wlr/types/wlr_surface.h>

struct hikari_view_interface {
  struct wlr_surface *(*surface_at)(
      struct hikari_view_interface *view_interface,
      double ox,
      double oy,
      double *sx,
      double *sy);

  void (*focus)(struct hikari_view_interface *view_interface);

  void (*for_each_surface)(struct hikari_view_interface *view_interface,
      void (*func)(struct wlr_surface *, int, int, void *),
      void *data);
};

static inline struct wlr_surface *
hikari_view_interface_surface_at(struct hikari_view_interface *view_interface,
    double ox,
    double oy,
    double *sx,
    double *sy)
{
  return view_interface->surface_at(view_interface, ox, oy, sx, sy);
}

static inline void
hikari_view_interface_focus(struct hikari_view_interface *view_interface)
{
  assert(view_interface->focus != NULL);
  view_interface->focus(view_interface);
}

static inline void
hikari_view_interface_for_each_surface(
    struct hikari_view_interface *view_interface,
    void (*func)(struct wlr_surface *, int, int, void *),
    void *data)
{
  view_interface->for_each_surface(view_interface, func, data);
}

#endif
