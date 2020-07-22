#if !defined(HIKARI_LAYER_SHELL_H)
#define HIKARI_LAYER_SHELL_H

#include <wlr/types/wlr_layer_shell_v1.h>

#include <hikari/node.h>

struct hikari_output;
struct hikari_layer;
struct hikari_layer_popup;

enum hikari_layer_node_type {
  HIKARI_LAYER_NODE_TYPE_LAYER,
  HIKARI_LAYER_NODE_TYPE_POPUP
};

struct hikari_layer_node {
  enum hikari_layer_node_type type;

  union {
    struct hikari_layer *layer;
    struct hikari_layer_popup *popup;
  } node;
};

struct hikari_layer {
  struct hikari_node node;

  struct wl_list layer_surfaces;

  struct wlr_layer_surface_v1 *surface;

  struct wl_listener commit;
  struct wl_listener destroy;
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener new_popup;

  struct wlr_box geometry;

  struct hikari_output *output;
  enum zwlr_layer_shell_v1_layer layer;
  bool mapped;
};

struct hikari_layer_popup {
  struct hikari_layer_node parent;

  struct wlr_xdg_popup *popup;

  struct wl_listener commit;
  struct wl_listener destroy;
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener new_popup;
};

void
hikari_layer_init(struct hikari_layer *layer_surface,
    struct wlr_layer_surface_v1 *wlr_layer_surface);

void
hikari_layer_fini(struct hikari_layer *layer_surface);

#endif
