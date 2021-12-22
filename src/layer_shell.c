#ifdef HAVE_LAYERSHELL
#include <hikari/layer_shell.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>

#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/server.h>

static void
map(struct hikari_layer *layer);

static void
map_handler(struct wl_listener *listener, void *data);

static void
unmap(struct hikari_layer *layer);

static void
unmap_handler(struct wl_listener *listener, void *data);

static void
commit_handler(struct wl_listener *listener, void *data);

static void
destroy_handler(struct wl_listener *listener, void *data);

static void
new_popup_handler(struct wl_listener *listener, void *data);

static void
for_each_surface(struct hikari_node *node,
    void (*func)(struct wlr_surface *, int, int, void *),
    void *data);

static struct wlr_surface *
surface_at(
    struct hikari_node *node, double ox, double oy, double *sx, double *sy);

static void
focus(struct hikari_node *node);

static void
calculate_geometry(struct hikari_layer *layer);

static void
init_layer_popup(struct hikari_layer_popup *layer_popup,
    struct hikari_layer *parent,
    struct wlr_xdg_popup *popup);

static void
init_popup_popup(struct hikari_layer_popup *layer_popup,
    struct hikari_layer_popup *parent,
    struct wlr_xdg_popup *popup);

static void
fini_popup(struct hikari_layer_popup *layer_popup);

static void
commit_popup_handler(struct wl_listener *listener, void *data);

static void
destroy_popup_handler(struct wl_listener *listener, void *data);

static void
map_popup_handler(struct wl_listener *listener, void *data);

static void
unmap_popup_handler(struct wl_listener *listener, void *data);

static void
destroy_popup_handler(struct wl_listener *listener, void *data);

static void
new_popup_popup_handler(struct wl_listener *listener, void *data);

static struct hikari_layer *
get_layer(struct hikari_layer_popup *popup);

static void
apply_layer_state(struct wlr_box *usable_area,
    uint32_t anchor,
    int32_t exclusive,
    int32_t margin_top,
    int32_t margin_right,
    int32_t margin_bottom,
    int32_t margin_left)
{
  if (exclusive <= 0) {
    return;
  }
  struct {
    uint32_t anchors;
    int *positive_axis;
    int *negative_axis;
    int margin;
  } edges[] = {
    {
        .anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
        .positive_axis = &usable_area->y,
        .negative_axis = &usable_area->height,
        .margin = margin_top,
    },
    {
        .anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
        .positive_axis = NULL,
        .negative_axis = &usable_area->height,
        .margin = margin_bottom,
    },
    {
        .anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
        .positive_axis = &usable_area->x,
        .negative_axis = &usable_area->width,
        .margin = margin_left,
    },
    {
        .anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
        .positive_axis = NULL,
        .negative_axis = &usable_area->width,
        .margin = margin_right,
    },
  };
  for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i) {
    if ((anchor & edges[i].anchors) == edges[i].anchors &&
        exclusive + edges[i].margin > 0) {
      if (edges[i].positive_axis) {
        *edges[i].positive_axis += exclusive + edges[i].margin;
      }
      if (edges[i].negative_axis) {
        *edges[i].negative_axis -= exclusive + edges[i].margin;
      }
    }
  }
}

static void
apply_state_for_layer(struct hikari_output *output,
    enum zwlr_layer_shell_v1_layer wlr_layer,
    struct wlr_box *usable_area)
{
  struct hikari_layer *layer;

  wl_list_for_each (layer, &output->layers[wlr_layer], layer_surfaces) {
    struct wlr_layer_surface_v1 *wlr_layer = layer->surface;
    struct wlr_layer_surface_v1_state *state = &wlr_layer->current;

    apply_layer_state(usable_area,
        state->anchor,
        state->exclusive_zone,
        state->margin.top,
        state->margin.right,
        state->margin.bottom,
        state->margin.left);
  }
}

static void
calculate_exclusive(struct hikari_output *output)
{
  assert(output != NULL);

  struct wlr_box usable_area = { 0 };

  wlr_output_effective_resolution(
      output->wlr_output, &usable_area.width, &usable_area.height);

  apply_state_for_layer(output, ZWLR_LAYER_SHELL_V1_LAYER_TOP, &usable_area);
  apply_state_for_layer(output, ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM, &usable_area);

  output->usable_area = usable_area;
}

void
hikari_layer_init(
    struct hikari_layer *layer, struct wlr_layer_surface_v1 *wlr_layer_surface)
{
#ifndef NDEBUG
  printf("LAYER INIT %p\n", layer);
#endif

  struct hikari_output *output = wlr_layer_surface->output != NULL
                                     ? wlr_layer_surface->output->data
                                     : hikari_server.workspace->output;

  layer->node.surface_at = surface_at;
  layer->node.focus = focus;
  layer->node.for_each_surface = for_each_surface;
  layer->output = output;
  layer->layer = wlr_layer_surface->pending.layer;
  layer->surface = wlr_layer_surface;
  layer->mapped = false;

  wlr_layer_surface->output = output->wlr_output;

  layer->commit.notify = commit_handler;
  wl_signal_add(&wlr_layer_surface->surface->events.commit, &layer->commit);

  layer->destroy.notify = destroy_handler;
  wl_signal_add(&wlr_layer_surface->surface->events.destroy, &layer->destroy);

  layer->map.notify = map_handler;
  wl_signal_add(&wlr_layer_surface->events.map, &layer->map);

  wl_list_insert(&output->layers[layer->layer], &layer->layer_surfaces);

  calculate_geometry(layer);
}

void
hikari_layer_fini(struct hikari_layer *layer)
{
  assert(!layer->mapped);

  wl_list_remove(&layer->layer_surfaces);

  wl_list_remove(&layer->commit.link);
  wl_list_remove(&layer->destroy.link);
  wl_list_remove(&layer->map.link);
}

static void
popup_unconstrain(struct hikari_layer_popup *layer_popup)
{
  struct hikari_layer *layer = get_layer(layer_popup);
  struct hikari_output *output = layer->output;

  struct wlr_box box = { .x = -layer->geometry.x,
    .y = -layer->geometry.y,
    .width = output->geometry.width,
    .height = output->geometry.height };

  wlr_xdg_popup_unconstrain_from_box(layer_popup->popup, &box);
}

static void
init_popup(
    struct hikari_layer_popup *layer_popup, struct wlr_xdg_popup *wlr_popup)
{
  layer_popup->popup = wlr_popup;

  layer_popup->commit.notify = commit_popup_handler;
  wl_signal_add(&wlr_popup->base->surface->events.commit, &layer_popup->commit);

  layer_popup->map.notify = map_popup_handler;
  wl_signal_add(&wlr_popup->base->events.map, &layer_popup->map);

  layer_popup->unmap.notify = unmap_popup_handler;
  wl_signal_add(&wlr_popup->base->events.unmap, &layer_popup->unmap);

  layer_popup->destroy.notify = destroy_popup_handler;
  wl_signal_add(&wlr_popup->base->events.destroy, &layer_popup->destroy);

  layer_popup->new_popup.notify = new_popup_popup_handler;
  wl_signal_add(&wlr_popup->base->events.new_popup, &layer_popup->new_popup);

  popup_unconstrain(layer_popup);
}

static struct hikari_layer *
get_layer(struct hikari_layer_popup *layer_popup)
{
  struct hikari_layer_popup *current = layer_popup;
  for (;;) {
    switch (current->parent.type) {
      case HIKARI_LAYER_NODE_TYPE_LAYER:
        return current->parent.node.layer;

      case HIKARI_LAYER_NODE_TYPE_POPUP:
        current = current->parent.node.popup;
        break;
    }
  }
}

static void
damage(struct hikari_layer *layer, bool whole)
{
  struct wlr_surface *surface = layer->surface->surface;

  if (whole) {
    struct wlr_box geometry = { .x = layer->geometry.x,
      .y = layer->geometry.y,
      .width = surface->current.width,
      .height = surface->current.height };

    hikari_output_add_damage(layer->output, &geometry);
  } else {
    hikari_output_add_effective_surface_damage(
        layer->output, surface, layer->geometry.x, layer->geometry.y);
  }
}

static void
damage_popup(struct hikari_layer_popup *layer_popup, bool whole)
{
  struct wlr_xdg_popup *popup = layer_popup->popup;
  struct wlr_surface *surface = popup->base->surface;

  int popup_sx = popup->geometry.x - popup->base->current.geometry.x;
  int popup_sy = popup->geometry.y - popup->base->current.geometry.y;
  int ox = popup_sx, oy = popup_sy;

  struct hikari_layer *layer;
  struct hikari_layer_popup *current = layer_popup;
  for (;;) {
    switch (current->parent.type) {
      case HIKARI_LAYER_NODE_TYPE_LAYER:
        layer = current->parent.node.layer;
        ox += layer->geometry.x;
        oy += layer->geometry.y;
        goto done;

      case HIKARI_LAYER_NODE_TYPE_POPUP:
        current = current->parent.node.popup;
        ox += current->popup->geometry.x;
        oy += current->popup->geometry.y;
        break;
    }
  }

done:

  assert(layer != NULL);

  struct hikari_output *output = layer->output;

  if (whole) {
    struct wlr_box geometry = { .x = ox,
      .y = oy,
      .width = surface->current.width,
      .height = surface->current.height };

    hikari_output_add_damage(output, &geometry);
  } else {
    hikari_output_add_effective_surface_damage(layer->output, surface, ox, oy);
  }
}

static void
commit_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer *layer = wl_container_of(listener, layer, commit);
  struct wlr_box old_geometry = layer->geometry;
  struct hikari_output *output = layer->output;

  if (!layer->mapped) {
    calculate_geometry(layer);
    return;
  }

  assert(layer->mapped);

  calculate_exclusive(layer->output);
  calculate_geometry(layer);

  enum zwlr_layer_shell_v1_layer current_layer = layer->surface->current.layer;
  bool updated_geometry =
      memcmp(&old_geometry, &layer->geometry, sizeof(struct wlr_box)) != 0;
  bool changed_layer = layer->layer != current_layer;

  if (changed_layer) {
    wl_list_remove(&layer->layer_surfaces);
    wl_list_insert(&output->layers[current_layer], &layer->layer_surfaces);
    layer->layer = current_layer;
  }

  if (updated_geometry || changed_layer) {
    hikari_output_add_damage(output, &old_geometry);
    hikari_output_add_damage(output, &layer->geometry);

    hikari_server_cursor_focus();
  } else {
    damage(layer, false);
  }
}

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer *layer = wl_container_of(listener, layer, destroy);

#ifndef NDEBUG
  printf("LAYER DESTROY %p\n", layer);
#endif

  if (layer->mapped) {
    unmap(layer);
  }

  assert(!layer->mapped);

  hikari_layer_fini(layer);
  hikari_free(layer);
}

static void
map(struct hikari_layer *layer)
{
#ifndef NDEBUG
  printf("LAYER MAP %p\n", layer);
#endif

  assert(!layer->mapped);

  struct wlr_layer_surface_v1 *wlr_layer_surface = layer->surface;

  layer->new_popup.notify = new_popup_handler;
  wl_signal_add(&wlr_layer_surface->events.new_popup, &layer->new_popup);

  layer->unmap.notify = unmap_handler;
  wl_signal_add(&wlr_layer_surface->events.unmap, &layer->unmap);

  wl_list_remove(&layer->map.link);

  layer->mapped = true;

  damage(layer, true);

  hikari_server_cursor_focus();
}

static void
map_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer *layer = wl_container_of(listener, layer, map);

  map(layer);
}

static void
unmap(struct hikari_layer *layer)
{
#ifndef NDEBUG
  printf("LAYER UNMAP %p\n", layer);
#endif

  struct hikari_workspace *workspace = layer->output->workspace;

  assert(layer->mapped);

  wl_list_remove(&layer->layer_surfaces);
  wl_list_init(&layer->layer_surfaces);

  wl_list_remove(&layer->new_popup.link);
  wl_list_remove(&layer->unmap.link);

  layer->map.notify = map_handler;
  wl_signal_add(&layer->surface->events.map, &layer->map);

  layer->mapped = false;

  damage(layer, true);

  calculate_exclusive(layer->output);

  if (layer == workspace->focus_layer) {
    struct wlr_seat *seat = hikari_server.seat;

    workspace->focus_layer = NULL;
    wlr_seat_keyboard_clear_focus(seat);
  }

  hikari_server_cursor_focus();
}

static void
unmap_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer *layer = wl_container_of(listener, layer, unmap);

  unmap(layer);
}

static void
calculate_geometry(struct hikari_layer *layer)
{
  struct hikari_output *output = layer->output;
  struct wlr_layer_surface_v1_state *state = &layer->surface->current;

  struct wlr_box geometry = { .x = 0,
    .y = 0,
    .width = state->desired_width,
    .height = state->desired_height };

  struct wlr_box bounds = { .x = 0,
    .y = 0,
    .width = output->geometry.width,
    .height = output->geometry.height };

  const uint32_t both_horiz =
      ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
  if ((state->anchor & both_horiz) && geometry.width == 0) {
    geometry.x = bounds.x;
    geometry.width = bounds.width;
  } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)) {
    geometry.x = bounds.x;
  } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT)) {
    geometry.x = bounds.x + (bounds.width - geometry.width);
  } else {
    geometry.x = bounds.x + ((bounds.width / 2) - (geometry.width / 2));
  }

  const uint32_t both_vert =
      ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
  if ((state->anchor & both_vert) && geometry.height == 0) {
    geometry.y = bounds.y;
    geometry.height = bounds.height;
  } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)) {
    geometry.y = bounds.y;
  } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM)) {
    geometry.y = bounds.y + (bounds.height - geometry.height);
  } else {
    geometry.y = bounds.y + ((bounds.height / 2) - (geometry.height / 2));
  }

  if ((state->anchor & both_horiz) == both_horiz) {
    geometry.x += state->margin.left;
    geometry.width -= state->margin.left + state->margin.right;
  } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT)) {
    geometry.x += state->margin.left;
  } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT)) {
    geometry.x -= state->margin.right;
  }
  if ((state->anchor & both_vert) == both_vert) {
    geometry.y += state->margin.top;
    geometry.height -= state->margin.top + state->margin.bottom;
  } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)) {
    geometry.y += state->margin.top;
  } else if ((state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM)) {
    geometry.y -= state->margin.bottom;
  }

  layer->geometry = geometry;

  wlr_layer_surface_v1_configure(
      layer->surface, geometry.width, geometry.height);
}

static void
destroy_popup_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer_popup *layer_popup =
      wl_container_of(listener, layer_popup, destroy);

#ifndef NDEBUG
  printf("DESTROY LAYER POPUP %p\n", layer_popup);
#endif

  fini_popup(layer_popup);

  hikari_free(layer_popup);
}

static void
map_popup_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer_popup *layer_popup =
      wl_container_of(listener, layer_popup, map);

#ifndef NDEBUG
  printf("MAP LAYER POPUP %p\n", layer_popup);
#endif

  damage_popup(layer_popup, true);
}

static void
unmap_popup_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer_popup *layer_popup =
      wl_container_of(listener, layer_popup, unmap);

#ifndef NDEBUG
  printf("UNMAP LAYER POPUP %p\n", layer_popup);
#endif

  damage_popup(layer_popup, true);
}

static void
commit_popup_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer_popup *layer_popup =
      wl_container_of(listener, layer_popup, commit);

  damage_popup(layer_popup, false);
}

static void
new_popup_popup_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer_popup *layer_popup =
      wl_container_of(listener, layer_popup, new_popup);

#ifndef NDEBUG
  printf("NEW LAYER POPUP POPUP %p\n", layer_popup);
#endif

  struct wlr_xdg_popup *wlr_popup = data;

  struct hikari_layer_popup *layer_popup_popup =
      hikari_malloc(sizeof(struct hikari_layer_popup));

  init_popup_popup(layer_popup_popup, layer_popup, wlr_popup);
}

static void
new_popup_handler(struct wl_listener *listener, void *data)
{
  struct hikari_layer *layer = wl_container_of(listener, layer, new_popup);

#ifndef NDEBUG
  printf("NEW LAYER POPUP\n");
#endif

  struct hikari_layer_popup *layer_popup =
      hikari_malloc(sizeof(struct hikari_layer_popup));

  struct wlr_xdg_popup *wlr_popup = data;

  init_layer_popup(layer_popup, layer, wlr_popup);
}

static void
focus(struct hikari_node *node)
{
  assert(node != NULL);

  struct hikari_layer *layer = (struct hikari_layer *)node;
  struct wlr_layer_surface_v1_state *state = &layer->surface->current;

  if (state->keyboard_interactive) {
    struct hikari_workspace *workspace = hikari_server.workspace;
    struct hikari_view *focus_view = workspace->focus_view;
    struct hikari_layer *focus_layer = workspace->focus_layer;
    struct wlr_seat *seat = hikari_server.seat;
    struct wlr_keyboard *wlr_keyboard = wlr_seat_get_keyboard(seat);

    if (focus_view != NULL) {
      hikari_workspace_focus_view(workspace, NULL);
    }

    if (focus_layer != NULL) {
      wlr_seat_keyboard_clear_focus(seat);
    }

    if (wlr_keyboard != NULL) {
      wlr_seat_keyboard_notify_enter(seat,
          layer->surface->surface,
          wlr_keyboard->keycodes,
          wlr_keyboard->num_keycodes,
          &wlr_keyboard->modifiers);
    }

    workspace->focus_layer = layer;
  }
}

static void
for_each_surface(struct hikari_node *node,
    void (*func)(struct wlr_surface *, int, int, void *),
    void *data)
{
  struct hikari_layer *layer = (struct hikari_layer *)node;

  wlr_layer_surface_v1_for_each_surface(layer->surface, func, data);
}

static struct wlr_surface *
surface_at(
    struct hikari_node *node, double ox, double oy, double *sx, double *sy)
{
  struct hikari_layer *layer = (struct hikari_layer *)node;

  double x = ox - layer->geometry.x;
  double y = oy - layer->geometry.y;

  struct wlr_surface *surface =
      wlr_layer_surface_v1_surface_at(layer->surface, x, y, sx, sy);

  return surface;
}

static void
init_layer_popup(struct hikari_layer_popup *layer_popup,
    struct hikari_layer *parent,
    struct wlr_xdg_popup *wlr_popup)
{
  layer_popup->parent.type = HIKARI_LAYER_NODE_TYPE_LAYER;
  layer_popup->parent.node.layer = parent;

  init_popup(layer_popup, wlr_popup);
}

static void
init_popup_popup(struct hikari_layer_popup *layer_popup,
    struct hikari_layer_popup *parent,
    struct wlr_xdg_popup *wlr_popup)
{
  layer_popup->parent.type = HIKARI_LAYER_NODE_TYPE_POPUP;
  layer_popup->parent.node.popup = parent;

  init_popup(layer_popup, wlr_popup);
}

static void
fini_popup(struct hikari_layer_popup *layer_popup)
{
  wl_list_remove(&layer_popup->commit.link);
  wl_list_remove(&layer_popup->destroy.link);
  wl_list_remove(&layer_popup->map.link);
  wl_list_remove(&layer_popup->unmap.link);
  wl_list_remove(&layer_popup->new_popup.link);
}

#endif
