#include <hikari/server.h>

#include <errno.h>
#include <libinput.h>
#include <unistd.h>

#include <wlr/backend.h>
#include <wlr/backend/headless.h>
#include <wlr/backend/libinput.h>
#include <wlr/backend/session.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>

#ifdef HAVE_LAYERSHELL
#include <wlr/types/wlr_layer_shell_v1.h>
#endif

#ifdef HAVE_GAMMACONTROL
#include <wlr/types/wlr_gamma_control_v1.h>
#endif

#ifdef HAVE_SCREENCOPY
#include <wlr/types/wlr_screencopy_v1.h>
#endif

#ifdef HAVE_XWAYLAND
#include <wlr/xwayland.h>
#endif

#include <hikari/border.h>
#include <hikari/command.h>
#include <hikari/configuration.h>
#include <hikari/decoration.h>
#include <hikari/exec.h>
#include <hikari/keyboard.h>
#include <hikari/layout.h>
#include <hikari/mark.h>
#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/pointer.h>
#include <hikari/pointer_config.h>
#include <hikari/sheet.h>
#include <hikari/switch.h>
#include <hikari/workspace.h>
#include <hikari/xdg_view.h>

#ifdef HAVE_XWAYLAND
#include <hikari/xwayland_unmanaged_view.h>
#include <hikari/xwayland_view.h>
#endif

static void
add_pointer(struct hikari_server *server, struct wlr_input_device *device)
{
  struct hikari_pointer *pointer = hikari_malloc(sizeof(struct hikari_pointer));
  hikari_pointer_init(pointer, device);

  struct hikari_pointer_config *pointer_config =
      hikari_configuration_resolve_pointer_config(
          hikari_configuration, device->name);

  if (pointer_config != NULL) {
    hikari_pointer_configure(pointer, pointer_config);
  }

  wlr_cursor_attach_input_device(server->cursor.wlr_cursor, device);
  wlr_cursor_map_input_to_output(server->cursor.wlr_cursor, device, NULL);
}

static void
add_keyboard(struct hikari_server *server, struct wlr_input_device *device)
{
  struct hikari_keyboard *keyboard =
      hikari_malloc(sizeof(struct hikari_keyboard));

  hikari_keyboard_init(keyboard, device);

  struct hikari_keyboard_config *keyboard_config =
      hikari_configuration_resolve_keyboard_config(
          hikari_configuration, device->name);

  assert(keyboard_config != NULL);
  hikari_keyboard_configure(keyboard, keyboard_config);

  hikari_keyboard_configure_bindings(
      keyboard, &hikari_configuration->keyboard_binding_configs);
}

static void
add_switch(struct hikari_server *server, struct wlr_input_device *device)
{
  struct hikari_switch *swtch = hikari_malloc(sizeof(struct hikari_switch));

  hikari_switch_init(swtch, device);

  struct hikari_switch_config *switch_config =
      hikari_configuration_resolve_switch_config(
          hikari_configuration, device->name);

  if (switch_config != NULL) {
    hikari_switch_configure(swtch, switch_config);
  }
}

static void
add_input(struct hikari_server *server, struct wlr_input_device *device)
{

  switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
      add_keyboard(server, device);
      break;

    case WLR_INPUT_DEVICE_POINTER:
      add_pointer(server, device);
      break;

    case WLR_INPUT_DEVICE_SWITCH:
      add_switch(server, device);
      break;

    default:
      break;
  }

  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
  if (!wl_list_empty(&server->keyboards)) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }
  wlr_seat_set_capabilities(server->seat, caps);

  if ((caps & WL_SEAT_CAPABILITY_POINTER) != 0) {
    hikari_cursor_reset_image(&server->cursor);
  }
}

static void
new_input_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server = wl_container_of(listener, server, new_input);
  struct wlr_input_device *device = data;

  add_input(server, device);
}

#ifdef HAVE_VIRTUAL_INPUT
static void
new_virtual_keyboard_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, new_virtual_keyboard);
  struct wlr_virtual_keyboard_v1 *keyboard = data;
  struct wlr_input_device *device = &keyboard->input_device;

  add_input(server, device);
}

static void
setup_virtual_keyboard(struct hikari_server *server)
{
  server->virtual_keyboard =
      wlr_virtual_keyboard_manager_v1_create(server->display);
  wl_signal_add(&server->virtual_keyboard->events.new_virtual_keyboard,
      &server->new_virtual_keyboard);
  server->new_virtual_keyboard.notify = new_virtual_keyboard_handler;
}

static void
new_virtual_pointer_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, new_virtual_pointer);
  struct wlr_virtual_pointer_v1_new_pointer_event *event = data;
  struct wlr_virtual_pointer_v1 *pointer = event->new_pointer;
  struct wlr_input_device *device = &pointer->input_device;

  add_input(server, device);

  if (event->suggested_output) {
    wlr_cursor_map_to_output(
        server->cursor.wlr_cursor, event->suggested_output);
  }
}

static void
setup_virtual_pointer(struct hikari_server *server)
{
  server->virtual_pointer =
      wlr_virtual_pointer_manager_v1_create(server->display);
  wl_signal_add(&server->virtual_pointer->events.new_virtual_pointer,
      &server->new_virtual_pointer);
  server->new_virtual_pointer.notify = new_virtual_pointer_handler;
}
#endif

static void
new_output_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server = wl_container_of(listener, server, new_output);

  assert(server == &hikari_server);

  struct wlr_output *wlr_output = data;
  struct hikari_output *output = hikari_malloc(sizeof(struct hikari_output));

  if (!wlr_output_init_render(
          wlr_output, server->allocator, server->renderer)) {
    exit(EXIT_FAILURE);
  }

  hikari_output_init(output, wlr_output);
  hikari_cursor_reset_image(&server->cursor);
}

static bool
surface_at(struct hikari_node *node,
    double ox,
    double oy,
    struct wlr_surface **surface,
    double *sx,
    double *sy)
{
  double out_sx, out_sy;

  struct wlr_surface *out_surface =
      hikari_node_surface_at(node, ox, oy, &out_sx, &out_sy);

  if (out_surface != NULL) {
    *sx = out_sx;
    *sy = out_sy;
    *surface = out_surface;
    return true;
  }

  return false;
}

#ifdef HAVE_LAYERSHELL
static bool
layer_at(struct wl_list *layers,
    double ox,
    double oy,
    struct wlr_surface **surface,
    double *sx,
    double *sy,
    struct hikari_node **node)
{
  double out_sx, out_sy;

  struct hikari_layer *layer;
  wl_list_for_each (layer, layers, layer_surfaces) {
    struct hikari_node *out_node = (struct hikari_node *)layer;

    struct wlr_surface *out_surface =
        hikari_node_surface_at(out_node, ox, oy, &out_sx, &out_sy);

    if (out_surface != NULL) {
      *sx = out_sx;
      *sy = out_sy;
      *surface = out_surface;
      *node = out_node;
      return true;
    }
  }

  return false;
}

static bool
topmost_of(struct wl_list *layers,
    double ox,
    double oy,
    struct wlr_surface **surface,
    double *sx,
    double *sy,
    struct hikari_node **node)
{
  double out_sx, out_sy;

  struct hikari_layer *layer;
  wl_list_for_each (layer, layers, layer_surfaces) {
    struct hikari_node *out_node = (struct hikari_node *)layer;

    struct wlr_layer_surface_v1_state *state = &layer->surface->current;

    struct wlr_surface *out_surface =
        hikari_node_surface_at(out_node, ox, oy, &out_sx, &out_sy);

    if (state->keyboard_interactive) {
      if (out_surface != NULL) {
        *surface = out_surface;
      } else {
        *surface = layer->surface->surface;
      }

      *sx = out_sx;
      *sy = out_sy;
      *node = out_node;

      return true;
    } else if (out_surface != NULL) {
      *surface = out_surface;

      *sx = out_sx;
      *sy = out_sy;
      *node = out_node;

      return true;
    }
  }

  return false;
}
#endif

static struct hikari_node *
node_at(double lx,
    double ly,
    struct wlr_surface **surface,
    struct hikari_workspace **workspace,
    double *sx,
    double *sy)
{
  assert(hikari_server.workspace != NULL);

  struct wlr_output *wlr_output =
      wlr_output_layout_output_at(hikari_server.output_layout, lx, ly);

  if (wlr_output == NULL) {
    *workspace = hikari_server.workspace;
    return NULL;
  }

  struct hikari_output *output = wlr_output->data;
  struct hikari_workspace *output_workspace = output->workspace;

  *workspace = output_workspace;

  struct hikari_node *node;
  double ox = lx - output->geometry.x;
  double oy = ly - output->geometry.y;

#ifdef HAVE_LAYERSHELL
  if (topmost_of(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY],
          ox,
          oy,
          surface,
          sx,
          sy,
          &node)) {
    return node;
  }
#endif

#ifdef HAVE_XWAYLAND
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view = NULL;
  wl_list_for_each (xwayland_unmanaged_view,
      &output->unmanaged_xwayland_views,
      unmanaged_output_views) {
    node = (struct hikari_node *)xwayland_unmanaged_view;

    if (surface_at(node, ox, oy, surface, sx, sy)) {
      return node;
    }
  }
#endif

#ifdef HAVE_LAYERSHELL
  if (topmost_of(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_TOP],
          ox,
          oy,
          surface,
          sx,
          sy,
          &node)) {
    return node;
  }
#endif

  struct hikari_view *view = NULL;
  wl_list_for_each (view, &output_workspace->views, workspace_views) {
    node = (struct hikari_node *)view;

    if (surface_at(node, ox, oy, surface, sx, sy)) {
      return node;
    }
  }

#ifdef HAVE_LAYERSHELL
  if (layer_at(&output->layers[ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM],
          ox,
          oy,
          surface,
          sx,
          sy,
          &node)) {
    return node;
  }
#endif

  return NULL;
}

struct hikari_node *
hikari_server_node_at(double x,
    double y,
    struct wlr_surface **surface,
    struct hikari_workspace **workspace,
    double *sx,
    double *sy)
{
  return node_at(x, y, surface, workspace, sx, sy);
}

void
hikari_server_cursor_focus(void)
{
  struct timespec now;
  uint32_t time_msec = (uint32_t)clock_gettime(CLOCK_MONOTONIC, &now);
  hikari_server.mode->cursor_move(time_msec);
}

static void
request_set_primary_selection_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, request_set_primary_selection);

  struct wlr_seat_request_set_primary_selection_event *event = data;

  // CAN FAIL WITH NULL POINTER. HOW?
  wlr_seat_set_primary_selection(server->seat, event->source, event->serial);
}

static void
request_set_selection_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, request_set_selection);

  struct wlr_seat_request_set_selection_event *event = data;

  wlr_seat_set_selection(server->seat, event->source, event->serial);
}

#ifdef HAVE_XWAYLAND
static void
new_xwayland_surface_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, new_xwayland_surface);

  struct wlr_xwayland_surface *wlr_xwayland_surface = data;

  struct hikari_workspace *workspace = server->workspace;

  if (wlr_xwayland_surface->override_redirect) {
    struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view =
        hikari_malloc(sizeof(struct hikari_xwayland_unmanaged_view));

    hikari_xwayland_unmanaged_view_init(
        xwayland_unmanaged_view, wlr_xwayland_surface, workspace);
  } else {
    struct hikari_xwayland_view *xwayland_view =
        hikari_malloc(sizeof(struct hikari_xwayland_view));

    hikari_xwayland_view_init(xwayland_view, wlr_xwayland_surface, workspace);
  }
}

static void
setup_xwayland(struct hikari_server *server)
{
  server->xwayland =
      wlr_xwayland_create(server->display, server->compositor, true);

  server->new_xwayland_surface.notify = new_xwayland_surface_handler;
  wl_signal_add(
      &server->xwayland->events.new_surface, &server->new_xwayland_surface);

  setenv("DISPLAY", server->xwayland->display_name, true);
}
#endif

static void
setup_cursor(struct hikari_server *server)
{
  hikari_cursor_init(&hikari_server.cursor, server->output_layout);

  hikari_cursor_configure_bindings(
      &hikari_server.cursor, &hikari_configuration->mouse_binding_configs);
}

static void
server_decoration_mode_handler(struct wl_listener *listener, void *data)
{
  struct hikari_view_decoration *decoration =
      wl_container_of(listener, decoration, mode);
  struct hikari_view *view = decoration->view;

  view->use_csd = decoration->wlr_decoration->mode ==
                  WLR_SERVER_DECORATION_MANAGER_MODE_CLIENT;

  if (view->use_csd) {
    view->border.state = HIKARI_BORDER_NONE;
    hikari_output_damage_whole(hikari_server.workspace->output);
  }
}

static void
server_decoration_handler(struct wl_listener *listener, void *data)
{
  struct wlr_server_decoration *wlr_decoration = data;
  struct hikari_view *view =
      wl_container_of(wlr_decoration->surface, view, surface);
  struct wlr_xdg_surface *xdg_surface =
      wlr_xdg_surface_from_wlr_surface(wlr_decoration->surface);
  struct hikari_xdg_view *xdg_view = xdg_surface->data;

  if (xdg_view == NULL) {
    return;
  }

  wl_signal_add(&wlr_decoration->events.mode, &xdg_view->view.decoration.mode);
  xdg_view->view.decoration.mode.notify = server_decoration_mode_handler;

  xdg_view->view.decoration.wlr_decoration = wlr_decoration;
  xdg_view->view.decoration.view = &xdg_view->view;
}

static void
new_toplevel_decoration_handler(struct wl_listener *listener, void *data)
{
  struct wlr_xdg_toplevel_decoration_v1 *wlr_decoration = data;

  struct hikari_decoration *decoration =
      hikari_malloc(sizeof(struct hikari_decoration));

  hikari_decoration_init(decoration, wlr_decoration);
}

static void
setup_decorations(struct hikari_server *server)
{
  server->decoration_manager =
      wlr_server_decoration_manager_create(server->display);

  wlr_server_decoration_manager_set_default_mode(
      server->decoration_manager, WLR_SERVER_DECORATION_MANAGER_MODE_SERVER);

  wl_signal_add(&server->decoration_manager->events.new_decoration,
      &server->new_decoration);
  server->new_decoration.notify = server_decoration_handler;

  server->xdg_decoration_manager =
      wlr_xdg_decoration_manager_v1_create(server->display);
  wl_signal_add(&server->xdg_decoration_manager->events.new_toplevel_decoration,
      &server->new_toplevel_decoration);
  server->new_toplevel_decoration.notify = new_toplevel_decoration_handler;
}

static void
start_drag_handler(struct wl_listener *listener, void *data)
{
  struct wlr_surface *surface;
  struct hikari_workspace *workspace;
  double sx, sy;

  struct hikari_node *node = node_at(hikari_server.cursor.wlr_cursor->x,
      hikari_server.cursor.wlr_cursor->y,
      &surface,
      &workspace,
      &sx,
      &sy);

  if (node != NULL) {
    hikari_dnd_mode_enter();
  }
}

static void
request_start_drag_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, request_start_drag);
  struct wlr_seat_request_start_drag_event *event = data;

  if (wlr_seat_validate_pointer_grab_serial(
          server->seat, event->origin, event->serial)) {
    wlr_seat_start_pointer_drag(server->seat, event->drag, event->serial);
  } else {
    wlr_data_source_destroy(event->drag->source);
  }
}

static void
setup_selection(struct hikari_server *server)
{
  wlr_data_control_manager_v1_create(server->display);

  wlr_primary_selection_v1_device_manager_create(server->display);

  server->seat = wlr_seat_create(server->display, "seat0");
  assert(server->seat != NULL);

  server->request_set_primary_selection.notify =
      request_set_primary_selection_handler;
  wl_signal_add(&server->seat->events.request_set_primary_selection,
      &server->request_set_primary_selection);

  server->request_set_selection.notify = request_set_selection_handler;
  wl_signal_add(&server->seat->events.request_set_selection,
      &server->request_set_selection);

  server->request_start_drag.notify = request_start_drag_handler;
  wl_signal_add(
      &server->seat->events.request_start_drag, &server->request_start_drag);

  server->start_drag.notify = start_drag_handler;
  wl_signal_add(&server->seat->events.start_drag, &server->start_drag);
}

static void
new_xdg_surface_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, new_xdg_surface);

  struct wlr_xdg_surface *xdg_surface = data;

  if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
    return;
  }

  struct hikari_xdg_view *xdg_view =
      hikari_malloc(sizeof(struct hikari_xdg_view));

  hikari_xdg_view_init(xdg_view, xdg_surface, server->workspace);
}

static void
setup_xdg_shell(struct hikari_server *server)
{
  server->xdg_shell = wlr_xdg_shell_create(server->display);

  server->new_xdg_surface.notify = new_xdg_surface_handler;
  wl_signal_add(
      &server->xdg_shell->events.new_surface, &server->new_xdg_surface);
}

#ifdef HAVE_LAYERSHELL
static void
new_layer_shell_surface_handler(struct wl_listener *listener, void *data)
{
  struct wlr_layer_surface_v1 *wlr_layer_surface =
      (struct wlr_layer_surface_v1 *)data;
  struct hikari_layer *layer = hikari_malloc(sizeof(struct hikari_layer));

  hikari_layer_init(layer, wlr_layer_surface);
}

static void
setup_layer_shell(struct hikari_server *server)
{
  server->layer_shell = wlr_layer_shell_v1_create(server->display);

  wl_signal_add(&server->layer_shell->events.new_surface,
      &server->new_layer_shell_surface);
  server->new_layer_shell_surface.notify = new_layer_shell_surface_handler;
}
#endif

struct hikari_server hikari_server;

static void
output_layout_change_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, output_layout_change);

  struct hikari_output *output;
  wl_list_for_each (output, &server->outputs, server_outputs) {
    struct wlr_output *wlr_output = output->wlr_output;
    struct wlr_box *output_box =
        wlr_output_layout_get_box(hikari_server.output_layout, wlr_output);

    output->geometry.x = output_box->x;
    output->geometry.y = output_box->y;
    output->geometry.width = output_box->width;
    output->geometry.height = output_box->height;

    struct hikari_output_config *output_config =
        hikari_configuration_resolve_output_config(
            hikari_configuration, wlr_output->name);

    if (output_config != NULL) {
      hikari_output_load_background(output,
          output_config->background.value,
          output_config->background_fit.value);
    }

#ifdef HAVE_XWAYLAND
    hikari_output_rearrange_xwayland_views(output);
#endif
  }
}

static bool
drop_privileges(struct hikari_server *server)
{
  if (getuid() != geteuid() || getgid() != getegid()) {
    if (setuid(getuid()) != 0 || setgid(getgid()) != 0) {
      return false;
    }
  }

  if (geteuid() == 0 || getegid() == 0) {
    fprintf(stderr, "running as root is prohibited\n");
    return false;
  }

  return true;
}

void
hikari_server_prepare_privileged(void)
{
  bool success = false;
  struct hikari_server *server = &hikari_server;

  server->display = wl_display_create();
  if (server->display == NULL) {
    fprintf(stderr, "error: could not create display\n");
    goto done;
  }

  server->event_loop = wl_display_get_event_loop(server->display);
  if (server->event_loop == NULL) {
    fprintf(stderr, "error: could not create event loop\n");
    goto done;
  }

  server->backend = wlr_backend_autocreate(server->display);
  if (server->backend == NULL) {
    fprintf(stderr, "error: could not create backend\n");
    goto done;
  }

  success = true;

done:
  if (!drop_privileges(server) || !success) {
    if (server->display != NULL) {
      wl_display_destroy(server->display);
    }

    exit(EXIT_FAILURE);
  }
}

static void
init_noop_output(struct hikari_server *server)
{
  server->noop_backend = wlr_headless_backend_create(server->display);

  struct wlr_output *wlr_output =
      wlr_headless_add_output(server->noop_backend, 800, 600);
  struct hikari_output *noop_output =
      hikari_malloc(sizeof(struct hikari_output));

  server->noop_output = noop_output;
  hikari_output_init(noop_output, wlr_output);

  hikari_server.workspace = noop_output->workspace;
  hikari_cursor_activate(&hikari_server.cursor);
  hikari_server.mode = (struct hikari_mode *)&hikari_server.normal_mode;
}

static void
server_init(struct hikari_server *server, char *config_path)
{
#ifndef NDEBUG
  server->track_damage = false;
#endif
  server->shutdown_timer = NULL;
  server->config_path = config_path;

  hikari_configuration = hikari_malloc(sizeof(struct hikari_configuration));

  hikari_configuration_init(hikari_configuration);

  if (!hikari_configuration_load(hikari_configuration, config_path)) {
    hikari_configuration_fini(hikari_configuration);
    hikari_free(hikari_configuration);

    wl_display_destroy(server->display);
    exit(EXIT_FAILURE);
  }

  server->keyboard_state.modifiers = 0;
  server->keyboard_state.mod_released = false;
  server->keyboard_state.mod_changed = false;
  server->keyboard_state.mod_pressed = false;

  server->cycling = false;
  server->workspace = NULL;

  hikari_indicator_init(
      &server->indicator, hikari_configuration->indicator_selected);

  wl_list_init(&server->outputs);

  signal(SIGPIPE, SIG_IGN);

  server->renderer = wlr_renderer_autocreate(server->backend);
  if (server->renderer == NULL) {
    wl_display_destroy(server->display);
    exit(EXIT_FAILURE);
  }

  wlr_renderer_init_wl_display(server->renderer, server->display);

  server->allocator =
      wlr_allocator_autocreate(server->backend, server->renderer);
  if (server->allocator == NULL) {
    wl_display_destroy(server->display);
    exit(EXIT_FAILURE);
  }

  server->socket = wl_display_add_socket_auto(server->display);
  if (server->socket == NULL) {
    wl_display_destroy(server->display);
    exit(EXIT_FAILURE);
  }

  setenv("WAYLAND_DISPLAY", server->socket, true);

  server->compositor = wlr_compositor_create(server->display, server->renderer);

  server->data_device_manager = wlr_data_device_manager_create(server->display);

  server->new_input.notify = new_input_handler;
  wl_signal_add(&server->backend->events.new_input, &server->new_input);

  server->output_layout = wlr_output_layout_create();
  server->output_manager =
      wlr_xdg_output_manager_v1_create(server->display, server->output_layout);

  server->output_layout_change.notify = output_layout_change_handler;
  wl_signal_add(
      &server->output_layout->events.change, &server->output_layout_change);

  server->new_output.notify = new_output_handler;
  wl_signal_add(&server->backend->events.new_output, &server->new_output);

#ifdef HAVE_GAMMACONTROL
  wlr_gamma_control_manager_v1_create(server->display);
#endif

#ifdef HAVE_SCREENCOPY
  wlr_screencopy_manager_v1_create(server->display);
#endif

#ifdef HAVE_XWAYLAND
  setup_xwayland(server);
#endif
  setup_cursor(server);
#ifdef HAVE_VIRTUAL_INPUT
  setup_virtual_keyboard(server);
  setup_virtual_pointer(server);
#endif
  setup_decorations(server);
  setup_selection(server);
  setup_xdg_shell(server);
#ifdef HAVE_LAYERSHELL
  setup_layer_shell(server);
#endif

  wl_list_init(&server->pointers);
  wl_list_init(&server->keyboards);
  wl_list_init(&server->switches);

  wl_list_init(&server->groups);
  wl_list_init(&server->visible_groups);
  wl_list_init(&server->visible_views);

  hikari_dnd_mode_init(&server->dnd_mode);
  hikari_group_assign_mode_init(&server->group_assign_mode);
  hikari_input_grab_mode_init(&server->input_grab_mode);
  hikari_layout_select_mode_init(&server->layout_select_mode);
  hikari_lock_mode_init(&server->lock_mode);
  hikari_mark_assign_mode_init(&server->mark_assign_mode);
  hikari_mark_select_mode_init(&server->mark_select_mode);
  hikari_move_mode_init(&server->move_mode);
  hikari_normal_mode_init(&server->normal_mode);
  hikari_resize_mode_init(&server->resize_mode);
  hikari_sheet_assign_mode_init(&server->sheet_assign_mode);

  hikari_marks_init();

  init_noop_output(server);
}

static void
sig_handler(int signal)
{
  hikari_server_terminate(NULL);
}

static void
run_autostart(char *autostart)
{
  hikari_command_execute(autostart);
  free(autostart);
}

void
hikari_server_start(char *config_path, char *autostart)
{
  server_init(&hikari_server, config_path);
  signal(SIGTERM, sig_handler);

  wlr_backend_start(hikari_server.backend);

  if (autostart != NULL) {
    run_autostart(autostart);
  }

  wl_display_run(hikari_server.display);
}

static int
shutdown_handler(void *data)
{
  struct hikari_server *server = &hikari_server;

  if (server->shutdown_timer == NULL) {
    return 0;
  }

  struct hikari_output *output;
  wl_list_for_each (output, &server->outputs, server_outputs) {
    if (!wl_list_empty(&output->views)) {
      wl_event_source_timer_update(server->shutdown_timer, 1000);
      return 0;
    }
  }

  wl_display_terminate(hikari_server.display);

  return 0;
}

static void
destroy_shutdown_timer(struct hikari_server *server)
{
  wl_event_source_timer_update(server->shutdown_timer, 0);
  wl_event_source_remove(server->shutdown_timer);

  server->shutdown_timer = NULL;
}

void
hikari_server_terminate(void *arg)
{
  struct hikari_server *server = &hikari_server;

  if (server->shutdown_timer != NULL) {
    destroy_shutdown_timer(server);
    wl_display_terminate(server->display);
    return;
  }

  struct hikari_output *output;
  wl_list_for_each (output, &server->outputs, server_outputs) {
    struct hikari_view *view, *view_temp;
    wl_list_for_each_safe (view, view_temp, &output->views, output_views) {
      hikari_view_quit(view);
    }
  }

  server->shutdown_timer =
      wl_event_loop_add_timer(server->event_loop, shutdown_handler, NULL);

  wl_event_source_timer_update(server->shutdown_timer, 100);
}

void
hikari_server_stop(void)
{
  struct hikari_server *server = &hikari_server;

  wl_list_remove(&server->new_output.link);
  wl_list_remove(&server->new_input.link);
  wl_list_remove(&server->new_xdg_surface.link);
  wl_list_remove(&server->request_set_primary_selection.link);
  wl_list_remove(&server->request_start_drag.link);
  wl_list_remove(&server->start_drag.link);
  wl_list_remove(&server->output_layout_change.link);
#ifdef HAVE_XWAYLAND
  wl_list_remove(&server->new_xwayland_surface.link);
#endif

  if (server->shutdown_timer != NULL) {
    destroy_shutdown_timer(server);
  }

  hikari_cursor_fini(&server->cursor);
  hikari_indicator_fini(&server->indicator);

  hikari_lock_mode_fini(&server->lock_mode);
  hikari_mark_assign_mode_fini(&server->mark_assign_mode);

  wl_display_destroy_clients(server->display);

#if HAVE_XWAYLAND
  wlr_xwayland_destroy(server->xwayland);
#endif

  wlr_seat_destroy(server->seat);
  wl_display_destroy(server->display);
  wlr_output_layout_destroy(server->output_layout);

  hikari_configuration_fini(hikari_configuration);
  hikari_free(hikari_configuration);
  hikari_marks_fini();

  free(server->config_path);
}

struct hikari_group *
hikari_server_find_group(const char *group_name)
{
  struct hikari_group *group;
  wl_list_for_each (group, &hikari_server.groups, server_groups) {
    if (!strcmp(group_name, group->name)) {
      return group;
    }
  }

  return NULL;
}

struct hikari_group *
hikari_server_find_or_create_group(const char *group_name)
{
  struct hikari_group *group = hikari_server_find_group(group_name);

  if (group == NULL) {
    group = hikari_malloc(sizeof(struct hikari_group));
    hikari_group_init(group, group_name);
  }

  return group;
}

void
hikari_server_lock(void *arg)
{
  hikari_lock_mode_enter();
}

void
hikari_server_reload(void *arg)
{
  hikari_configuration_reload(hikari_server.config_path);
}

#define CYCLE_VIEW(name, link)                                                 \
  static struct hikari_view *cycle_##name##_view(void)                         \
  {                                                                            \
    struct hikari_server *server = &hikari_server;                             \
                                                                               \
    if (wl_list_empty(&server->visible_views)) {                               \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    struct hikari_view *focus_view = server->workspace->focus_view;            \
                                                                               \
    struct hikari_view *view;                                                  \
    if (focus_view == NULL) {                                                  \
      view = wl_container_of(                                                  \
          server->visible_views.link, view, visible_server_views);             \
    } else {                                                                   \
      struct wl_list *link = focus_view->visible_server_views.link;            \
      if (link != &server->visible_views) {                                    \
        view = wl_container_of(link, view, visible_server_views);              \
      } else {                                                                 \
        view = wl_container_of(                                                \
            server->visible_views.link, view, visible_server_views);           \
      }                                                                        \
    }                                                                          \
                                                                               \
    return view;                                                               \
  }                                                                            \
                                                                               \
  void hikari_server_cycle_##name##_view(void *arg)                            \
  {                                                                            \
    struct hikari_view *view;                                                  \
                                                                               \
    hikari_server_set_cycling();                                               \
    view = cycle_##name##_view();                                              \
                                                                               \
    if (view != NULL && view != hikari_server.workspace->focus_view) {         \
      hikari_workspace_focus_view(view->sheet->workspace, view);               \
    }                                                                          \
  }

CYCLE_VIEW(next, prev)
CYCLE_VIEW(prev, next)
#undef CYCLE_VIEW

#define CYCLE_ACTION(n)                                                        \
  void hikari_server_cycle_##n(void *arg)                                      \
  {                                                                            \
    struct hikari_view *view;                                                  \
                                                                               \
    hikari_server_set_cycling();                                               \
    view = hikari_workspace_##n(hikari_server.workspace);                      \
                                                                               \
    if (view != NULL && view != hikari_server.workspace->focus_view) {         \
      hikari_workspace_focus_view(view->sheet->workspace, view);               \
    }                                                                          \
  }

CYCLE_ACTION(first_group_view)
CYCLE_ACTION(last_group_view)
CYCLE_ACTION(next_group_view)
CYCLE_ACTION(prev_group_view)
CYCLE_ACTION(next_layout_view)
CYCLE_ACTION(prev_layout_view)
CYCLE_ACTION(first_layout_view)
CYCLE_ACTION(last_layout_view)
CYCLE_ACTION(next_group)
CYCLE_ACTION(prev_group)
#undef CYCLE_ACTION

#define CYCLE_WORKSPACE(link)                                                  \
  void hikari_server_cycle_##link##_workspace(void *arg)                       \
  {                                                                            \
    struct hikari_workspace *workspace = hikari_server.workspace;              \
    struct hikari_workspace *link = hikari_workspace_##link(workspace);        \
                                                                               \
    if (workspace != link) {                                                   \
      hikari_server_set_cycling();                                             \
                                                                               \
      struct hikari_view *view = hikari_workspace_first_view(link);            \
      if (view != NULL) {                                                      \
        hikari_workspace_focus_view(link, view);                               \
        hikari_view_center_cursor(view);                                       \
      } else {                                                                 \
        hikari_workspace_center_cursor(link);                                  \
        hikari_server_cursor_focus();                                          \
      }                                                                        \
    }                                                                          \
  }

CYCLE_WORKSPACE(next)
CYCLE_WORKSPACE(prev)
#undef CYCLE_WORKSPACE

static void
update_indication(struct hikari_view *view)
{
  assert(view != NULL);
  assert(view->group != NULL);

  hikari_group_damage(view->group);
  hikari_indicator_damage(&hikari_server.indicator, view);
}

void
hikari_server_enter_normal_mode(void *arg)
{
  struct hikari_server *server = &hikari_server;

  hikari_cursor_reset_image(&server->cursor);

  hikari_normal_mode_enter();

  hikari_server_cursor_focus();
}

void
hikari_server_enter_sheet_assign_mode(void *arg)
{
  assert(hikari_server.workspace != NULL);

  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *focus_view = workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  update_indication(focus_view);

  hikari_sheet_assign_mode_enter(focus_view);
}

void
hikari_server_enter_move_mode(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  update_indication(focus_view);

  hikari_move_mode_enter(focus_view);
}

void
hikari_server_enter_resize_mode(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  update_indication(focus_view);

  hikari_resize_mode_enter(focus_view);
}

void
hikari_server_enter_group_assign_mode(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  hikari_group_assign_mode_enter(focus_view);
}

void
hikari_server_enter_input_grab_mode(void *arg)
{
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *focus_view = workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  update_indication(focus_view);

  hikari_input_grab_mode_enter(focus_view);
}

void
hikari_server_enter_mark_select_mode(void *arg)
{
  hikari_mark_select_mode_enter(false);
}

void
hikari_server_enter_mark_select_switch_mode(void *arg)
{
  hikari_mark_select_mode_enter(true);
}

void
hikari_server_enter_layout_select_mode(void *arg)
{
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *focus_view = workspace->focus_view;

  if (focus_view != NULL) {
    update_indication(focus_view);
  }

  hikari_layout_select_mode_enter();
}

void
hikari_server_enter_mark_assign_mode(void *arg)
{
  assert(hikari_server.workspace != NULL);

  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_view *focus_view = workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  update_indication(focus_view);

  hikari_mark_assign_mode_enter(focus_view);
}

void
hikari_server_execute_command(void *arg)
{
  const char *command = arg;
  hikari_command_execute(command);
}

void
hikari_server_reset_sheet_layout(void *arg)
{
  struct hikari_layout *layout = hikari_server.workspace->sheet->layout;

  if (layout == NULL) {
    return;
  }

  hikari_layout_reset(layout);
}

void
hikari_server_layout_restack_append(void *arg)
{
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_sheet *sheet = workspace->sheet;
  struct hikari_layout *layout = sheet->layout;

  if (layout == NULL) {
    struct hikari_split *split = hikari_sheet_default_split(sheet);

    if (split != NULL) {
      hikari_sheet_apply_split(sheet, split);
    }
  } else {
    hikari_workspace_display_sheet_current(workspace);
    hikari_layout_restack_append(layout);
  }
}

void
hikari_server_layout_restack_prepend(void *arg)
{
  struct hikari_workspace *workspace = hikari_server.workspace;
  struct hikari_sheet *sheet = workspace->sheet;
  struct hikari_layout *layout = sheet->layout;

  if (layout == NULL) {
    struct hikari_split *split = hikari_sheet_default_split(sheet);

    if (split != NULL) {
      hikari_sheet_apply_split(sheet, split);
    }
  } else {
    hikari_workspace_display_sheet_current(workspace);
    hikari_layout_restack_prepend(layout);
  }
}

void
hikari_server_layout_sheet(void *arg)
{
  char layout_register = (intptr_t)arg;

  struct hikari_split *split =
      hikari_configuration_lookup_layout(hikari_configuration, layout_register);

  if (split != NULL) {
    hikari_workspace_apply_split(hikari_server.workspace, split);
  }
}

void
hikari_server_session_change_vt(void *arg)
{
  const intptr_t vt = (intptr_t)arg;
  assert(vt >= 1 && vt <= 12);

  struct wlr_session *session = wlr_backend_get_session(hikari_server.backend);
  if (session != NULL) {
    wlr_session_change_vt(session, vt);
  }
}

static void
show_marked_view(struct hikari_view *view, struct hikari_mark *mark)
{
  if (view != NULL) {
    if (hikari_view_is_hidden(view)) {
      hikari_view_show(view);
    } else {
      hikari_view_raise(view);
    }

    hikari_view_center_cursor(view);
    hikari_server_cursor_focus();
  } else {
    char *command = hikari_configuration->execs[mark->nr].command;

    if (command != NULL) {
      hikari_command_execute(command);
    }
  }
}

void
hikari_server_show_mark(void *arg)
{
  assert(arg != NULL);

  struct hikari_mark *mark = (struct hikari_mark *)arg;
  struct hikari_view *view = mark->view;

  show_marked_view(view, mark);
}

void
hikari_server_switch_to_mark(void *arg)
{
  assert(arg != NULL);

  struct hikari_mark *mark = (struct hikari_mark *)arg;
  struct hikari_view *view = mark->view;

  if (view != NULL && view->sheet->workspace->sheet != view->sheet) {
    hikari_workspace_switch_sheet(view->sheet->workspace, view->sheet);
  }

  show_marked_view(view, mark);
}

void
hikari_server_migrate_focus_view(
    struct hikari_output *output, double lx, double ly, bool center)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  assert(focus_view != NULL);

  struct hikari_sheet *sheet = output->workspace->sheet;

  hikari_view_migrate(focus_view,
      sheet,
      lx - output->geometry.x,
      ly - output->geometry.y,
      center);

  hikari_indicator_update_sheet(
      &hikari_server.indicator, output, sheet, focus_view->flags);

  hikari_server.workspace->focus_view = NULL;
  hikari_server.workspace = output->workspace;
  hikari_server.workspace->focus_view = focus_view;
}

static void
move_view(int dx, int dy)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  struct hikari_output *view_output = focus_view->output;
  struct wlr_box *geometry = hikari_view_geometry(focus_view);

  double lx = view_output->geometry.x + geometry->x + dx;
  double ly = view_output->geometry.y + geometry->y + dy;

  struct wlr_output *wlr_output =
      wlr_output_layout_output_at(hikari_server.output_layout, lx, ly);

  hikari_server_set_cycling();

  if (wlr_output == NULL || wlr_output->data == view_output) {
    hikari_view_move(focus_view, dx, dy);
  } else {
    hikari_server_migrate_focus_view(wlr_output->data, lx, ly, false);
  }
}

void
hikari_server_move_view_up(void *arg)
{
  const int step = hikari_configuration->step;
  move_view(0, -step);
}

void
hikari_server_move_view_down(void *arg)
{
  const int step = hikari_configuration->step;
  move_view(0, step);
}

void
hikari_server_move_view_left(void *arg)
{
  const int step = hikari_configuration->step;
  move_view(-step, 0);
}

void
hikari_server_move_view_right(void *arg)
{
  const int step = hikari_configuration->step;
  move_view(step, 0);
}

static void
move_resize_view(int dx, int dy, int dwidth, int dheight)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  struct hikari_output *view_output = focus_view->output;
  struct wlr_box *geometry = hikari_view_geometry(focus_view);

  double lx = view_output->geometry.x + geometry->x + dy;
  double ly = view_output->geometry.y + geometry->y + dy;

  struct wlr_output *wlr_output =
      wlr_output_layout_output_at(hikari_server.output_layout, lx, ly);

  hikari_server_set_cycling();

  if (wlr_output == NULL || wlr_output->data == view_output) {
    hikari_view_move_resize(focus_view, dx, dy, dwidth, dheight);
  } else {
    hikari_server_migrate_focus_view(wlr_output->data, lx, ly, false);
    hikari_view_resize(focus_view, dheight, dwidth);
  }
}

void
hikari_server_decrease_view_size_down(void *arg)
{
  const int step = hikari_configuration->step;
  move_resize_view(0, step, 0, -step);
}

void
hikari_server_decrease_view_size_right(void *arg)
{
  const int step = hikari_configuration->step;
  move_resize_view(step, 0, -step, 0);
}

void
hikari_server_increase_view_size_up(void *arg)
{
  const int step = hikari_configuration->step;
  move_resize_view(0, -step, 0, step);
}

void
hikari_server_increase_view_size_left(void *arg)
{
  const int step = hikari_configuration->step;
  move_resize_view(-step, 0, step, 0);
}

void
hikari_server_lower_group(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  struct hikari_group *group = focus_view->group;

  hikari_group_lower(group, focus_view);
  hikari_server_cursor_focus();
}

void
hikari_server_raise_group(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  struct hikari_group *group = focus_view->group;

  hikari_group_raise(group, focus_view);
}

void
hikari_server_only_group(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  struct hikari_group *group = focus_view->group;

  struct hikari_output *output;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    hikari_workspace_clear(output->workspace);
  }

  hikari_group_show(group);
  hikari_server_cursor_focus();
}

void
hikari_server_hide_group(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  struct hikari_group *group = focus_view->group;
  assert(group != NULL);

  hikari_group_hide(group);
  hikari_server_cursor_focus();
}

#ifndef NDEBUG
void
hikari_server_toggle_damage_tracking(void *arg)
{
  hikari_server.track_damage = !hikari_server.track_damage;

  struct hikari_output *output = NULL;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    hikari_output_damage_whole(output);
  }
}
#endif
