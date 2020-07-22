#if !defined(HIKARI_NODE_H)
#define HIKARI_NODE_H

#include <assert.h>

#include <wlr/types/wlr_surface.h>

struct hikari_node {
  struct wlr_surface *(*surface_at)(
      struct hikari_node *node, double ox, double oy, double *sx, double *sy);

  void (*focus)(struct hikari_node *node);

  void (*for_each_surface)(struct hikari_node *node,
      void (*func)(struct wlr_surface *, int, int, void *),
      void *data);
};

static inline struct wlr_surface *
hikari_node_surface_at(
    struct hikari_node *node, double ox, double oy, double *sx, double *sy)
{
  return node->surface_at(node, ox, oy, sx, sy);
}

static inline void
hikari_node_focus(struct hikari_node *node)
{
  assert(node->focus != NULL);
  node->focus(node);
}

static inline void
hikari_node_for_each_surface(struct hikari_node *node,
    void (*func)(struct wlr_surface *, int, int, void *),
    void *data)
{
  node->for_each_surface(node, func, data);
}

#endif
