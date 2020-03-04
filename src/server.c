#include <hikari/server.h>

#include <libinput.h>
#include <unistd.h>

#include <wlr/backend.h>
#include <wlr/backend/libinput.h>
#include <wlr/backend/session.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_gtk_primary_selection.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#ifdef HAVE_XWAYLAND
#include <wlr/xwayland.h>
#endif

#include <hikari/border.h>
#include <hikari/command.h>
#include <hikari/configuration.h>
#include <hikari/exec.h>
#include <hikari/keyboard.h>
#include <hikari/mark.h>
#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/pointer.h>
#include <hikari/pointer_config.h>
#include <hikari/sheet.h>
#include <hikari/unlocker.h>
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

  wlr_cursor_attach_input_device(server->cursor, device);
  wlr_cursor_map_input_to_output(server->cursor, device, NULL);
}

static void
add_keyboard(struct hikari_server *server, struct wlr_input_device *device)
{
  struct hikari_keyboard *keyboard =
      hikari_malloc(sizeof(struct hikari_keyboard));

  hikari_keyboard_init(keyboard, device);
}

static void
set_cursor_image(struct hikari_server *server, const char *ptr)
{
  wlr_xcursor_manager_set_cursor_image(server->cursor_mgr, ptr, server->cursor);
}

static void
new_input_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server = wl_container_of(listener, server, new_input);

  struct wlr_input_device *device = data;

  switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
      add_keyboard(server, device);
      break;

    case WLR_INPUT_DEVICE_POINTER:
      add_pointer(server, device);
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
    set_cursor_image(server, "left_ptr");
  }
}

static void
new_output_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server = wl_container_of(listener, server, new_output);

  assert(server == &hikari_server);

  struct wlr_output *wlr_output = data;
  struct hikari_output *output = hikari_malloc(sizeof(struct hikari_output));

  hikari_output_init(output, wlr_output);
  if (hikari_server.workspace == NULL) {
    hikari_server.workspace = output->workspace;
    hikari_server_activate_cursor();
    hikari_server.mode = (struct hikari_mode *)&hikari_server.normal_mode;
  } else {
    set_cursor_image(server, "left_ptr");
  }
}

static bool
surface_at(struct hikari_view_interface *view_interface,
    double ox,
    double oy,
    struct wlr_surface **surface,
    double *sx,
    double *sy)
{
  double out_sx, out_sy;

  struct wlr_surface *out_surface = hikari_view_interface_surface_at(
      view_interface, ox, oy, &out_sx, &out_sy);

  if (out_surface != NULL) {
    *sx = out_sx;
    *sy = out_sy;
    *surface = out_surface;
    return true;
  }

  return false;
}

static struct hikari_view_interface *
view_interface_at(
    double lx, double ly, struct wlr_surface **surface, double *sx, double *sy)
{
  assert(hikari_server.workspace != NULL);

  struct wlr_output *wlr_output =
      wlr_output_layout_output_at(hikari_server.output_layout, lx, ly);

  if (wlr_output == NULL) {
    return NULL;
  }

  struct hikari_output *output = wlr_output->data;
  struct hikari_workspace *workspace;

  if (hikari_server.workspace != output->workspace) {
    if (hikari_server.workspace->focus_view != NULL) {
      hikari_workspace_focus_view(hikari_server.workspace, NULL);
    }

    workspace = output->workspace;
    hikari_server.workspace = workspace;

    if (workspace->focus_view != NULL) {
      hikari_workspace_focus_view(workspace, NULL);
    }
  } else {
    workspace = hikari_server.workspace;
  }

  double ox = lx - output->geometry.x;
  double oy = ly - output->geometry.y;

#ifdef HAVE_XWAYLAND
  struct hikari_xwayland_unmanaged_view *xwayland_unmanaged_view = NULL;
  wl_list_for_each (xwayland_unmanaged_view,
      &output->unmanaged_xwayland_views,
      unmanaged_server_views) {
    struct hikari_view_interface *view_interface =
        (struct hikari_view_interface *)xwayland_unmanaged_view;

    if (surface_at(view_interface, ox, oy, surface, sx, sy)) {
      return view_interface;
    }
  }
#endif

  struct hikari_view *view = NULL;
  wl_list_for_each (view, &workspace->views, workspace_views) {
    struct hikari_view_interface *view_interface =
        (struct hikari_view_interface *)view;

    if (surface_at(view_interface, ox, oy, surface, sx, sy)) {
      return view_interface;
    }
  }

  return NULL;
}

static void
cursor_focus(uint32_t time)
{
  assert(hikari_server_in_normal_mode());

  double sx, sy;
  struct wlr_seat *seat = hikari_server.seat;
  struct wlr_surface *surface = NULL;

  struct hikari_view_interface *view_interface = view_interface_at(
      hikari_server.cursor->x, hikari_server.cursor->y, &surface, &sx, &sy);

  struct hikari_workspace *workspace = hikari_server.workspace;

  if (view_interface) {
    struct hikari_view_interface *focus_view_interface =
        (struct hikari_view_interface *)workspace->focus_view;

    if (view_interface != focus_view_interface &&
        view_interface->focus != NULL) {
      hikari_view_interface_focus(view_interface);
    }

    bool mouse_focus_changed =
        hikari_server.seat->pointer_state.focused_surface != surface;

    wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
    if (!mouse_focus_changed) {
      wlr_seat_pointer_notify_motion(seat, time, sx, sy);
    }
  } else {
    wlr_seat_pointer_clear_focus(seat);
  }
}

void
hikari_server_cursor_focus(void)
{
  // TODO assert and check if we are in normal mode before we call this
  if (!hikari_server_in_normal_mode()) {
    return;
  }

  struct timespec now;
  uint32_t time = (uint32_t)clock_gettime(CLOCK_MONOTONIC, &now);
  cursor_focus(time);
}

static void
cursor_button_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, cursor_button);

  assert(!server->locked);

  hikari_server.mode->button_handler(listener, data);
}

static void
cursor_move(uint32_t time_msec)
{
  // this is cheaper than calling a function, since this happens quite often, we
  // for the common case.
  if (hikari_server.mode->cursor_move == NULL) {
    cursor_focus(time_msec);
  } else {
    hikari_server.mode->cursor_move();
  }
}

static void
cursor_motion_absolute_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, cursor_motion_absolute);

  assert(!server->locked);

  struct wlr_event_pointer_motion_absolute *event = data;

  wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);

  cursor_move(event->time_msec);
}

static void
cursor_motion_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, cursor_motion);

  assert(!server->locked);

  struct wlr_event_pointer_motion *event = data;

  wlr_cursor_move(
      server->cursor, event->device, event->delta_x, event->delta_y);

  cursor_move(event->time_msec);
}

static void
cursor_frame_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, cursor_frame);

  assert(!server->locked);

  wlr_seat_pointer_notify_frame(server->seat);
}

static void
cursor_axis_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server = wl_container_of(listener, server, cursor_axis);

  assert(!server->locked);

  struct wlr_event_pointer_axis *event = data;

  wlr_seat_pointer_notify_axis(server->seat,
      event->time_msec,
      event->orientation,
      event->delta,
      event->delta_discrete,
      event->source);
}

static void
request_set_primary_selection_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, request_set_primary_selection);

  struct wlr_seat_request_set_primary_selection_event *event = data;

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

void
hikari_server_activate_cursor(void)
{
  hikari_server.cursor_motion_absolute.notify = cursor_motion_absolute_handler;
  wl_signal_add(&hikari_server.cursor->events.motion_absolute,
      &hikari_server.cursor_motion_absolute);

  hikari_server.cursor_frame.notify = cursor_frame_handler;
  wl_signal_add(
      &hikari_server.cursor->events.frame, &hikari_server.cursor_frame);

  hikari_server.cursor_motion.notify = cursor_motion_handler;
  wl_signal_add(
      &hikari_server.cursor->events.motion, &hikari_server.cursor_motion);

  hikari_server.cursor_button.notify = cursor_button_handler;
  wl_signal_add(
      &hikari_server.cursor->events.button, &hikari_server.cursor_button);

  hikari_server.cursor_axis.notify = cursor_axis_handler;
  wl_signal_add(&hikari_server.cursor->events.axis, &hikari_server.cursor_axis);
}

void
hikari_server_deactivate_cursor(void)
{
  wl_list_remove(&hikari_server.cursor_motion_absolute.link);
  wl_list_remove(&hikari_server.cursor_frame.link);
  wl_list_remove(&hikari_server.cursor_motion.link);
  wl_list_remove(&hikari_server.cursor_button.link);
  wl_list_remove(&hikari_server.cursor_axis.link);
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
  server->cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(server->cursor, server->output_layout);

  char *cursor_theme = getenv("XCURSOR_THEME");

  server->cursor_mgr = wlr_xcursor_manager_create(cursor_theme, 24);
  wlr_xcursor_manager_load(server->cursor_mgr, 1);
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

  wl_signal_add(&wlr_decoration->events.mode, &xdg_view->view.decoration.mode);
  xdg_view->view.decoration.mode.notify = server_decoration_mode_handler;

  xdg_view->view.decoration.wlr_decoration = wlr_decoration;
  xdg_view->view.decoration.view = &xdg_view->view;
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
}

static void
setup_selection(struct hikari_server *server)
{
  wlr_gtk_primary_selection_device_manager_create(server->display);
  wlr_primary_selection_v1_device_manager_create(server->display);

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

struct hikari_server hikari_server;

static void
output_layout_change_handler(struct wl_listener *listener, void *data)
{
  struct hikari_server *server =
      wl_container_of(listener, server, output_layout_change);

  struct hikari_output *output = NULL;
  wl_list_for_each (output, &server->outputs, server_outputs) {
    struct wlr_box *output_box =
        wlr_output_layout_get_box(hikari_server.output_layout, output->output);
    output->geometry.x = output_box->x;
    output->geometry.y = output_box->y;
    output->geometry.width = output_box->width;
    output->geometry.height = output_box->height;
  }
}

static void
server_init(struct hikari_server *server)
{
#ifndef NDEBUG
  server->track_damage = false;
#endif

  hikari_configuration = hikari_malloc(sizeof(struct hikari_configuration));

  hikari_configuration_init(hikari_configuration);

  if (!hikari_configuration_load(hikari_configuration)) {
    hikari_configuration_fini(hikari_configuration);
    hikari_free(hikari_configuration);

    exit(EXIT_FAILURE);
  }

  server->keyboard_state.modifiers = 0;
  server->keyboard_state.mod_released = false;
  server->keyboard_state.mod_changed = false;
  server->keyboard_state.mod_pressed = false;

  server->locked = false;
  server->cycling = false;
  server->workspace = NULL;
  server->display = wl_display_create();
  assert(server->display);

  hikari_indicator_init(
      &server->indicator, hikari_configuration->indicator_selected);

  wl_list_init(&server->outputs);

  server->event_loop = wl_display_get_event_loop(server->display);
  assert(server->event_loop);

  server->backend = wlr_backend_autocreate(server->display, NULL);
  assert(server->backend);

  if (getuid() != geteuid() || getgid() != getegid()) {
    if (setuid(getuid()) != 0 || setgid(getgid()) != 0) {
      exit(EXIT_FAILURE);
    }
  }

  hikari_unlocker_init();

  signal(SIGPIPE, SIG_IGN);

  server->renderer = wlr_backend_get_renderer(server->backend);
  assert(server->renderer);

  wlr_renderer_init_wl_display(server->renderer, server->display);

  server->socket = wl_display_add_socket_auto(server->display);
  if (!server->socket) {
    return;
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

#ifdef HAVE_XWAYLAND
  setup_xwayland(server);
#endif
  setup_cursor(server);
  setup_decorations(server);
  setup_selection(server);
  setup_xdg_shell(server);

  wl_list_init(&server->keyboards);
  wl_list_init(&server->groups);
  wl_list_init(&server->visible_groups);
  wl_list_init(&server->workspaces);
  wl_list_init(&server->pointers);

  hikari_group_assign_mode_init(&server->group_assign_mode);
  hikari_keyboard_grab_mode_init(&server->keyboard_grab_mode);
  hikari_layout_select_mode_init(&server->layout_select_mode);
  hikari_mark_assign_mode_init(&server->mark_assign_mode);
  hikari_mark_select_mode_init(&server->mark_select_mode);
  hikari_move_mode_init(&server->move_mode);
  hikari_normal_mode_init(&server->normal_mode);
  hikari_resize_mode_init(&server->resize_mode);
  hikari_sheet_assign_mode_init(&server->sheet_assign_mode);
  hikari_tiling_mode_init(&server->tiling_mode);

  hikari_marks_init();
}

static void
sig_handler(int signal)
{
  hikari_server_terminate(NULL);
}

static char *
get_autostart_path(void)
{
  char *config_home = getenv("HOME");
  char *path = "/.config/hikari/autostart";
  size_t len = strlen(config_home) + strlen(path);

  char *ret = malloc(len + 1);

  memset(ret, 0, len + 1);

  strcat(ret, config_home);
  strcat(ret, path);

  return ret;
}

void
hikari_server_start(void)
{
  server_init(&hikari_server);
  signal(SIGTERM, sig_handler);
  wlr_backend_start(hikari_server.backend);

  char *autostart = get_autostart_path();
  hikari_command_execute(autostart);
  free(autostart);

  wl_display_run(hikari_server.display);
}

void
hikari_server_terminate(void *arg)
{
  wl_display_terminate(hikari_server.display);
}

void
hikari_server_stop(void)
{
  wl_display_destroy_clients(hikari_server.display);

  wl_list_remove(&hikari_server.new_output.link);
  wl_list_remove(&hikari_server.new_input.link);
  wl_list_remove(&hikari_server.new_xdg_surface.link);
  wl_list_remove(&hikari_server.cursor_motion_absolute.link);
  wl_list_remove(&hikari_server.cursor_motion.link);
  wl_list_remove(&hikari_server.cursor_frame.link);
  wl_list_remove(&hikari_server.cursor_axis.link);
  wl_list_remove(&hikari_server.cursor_button.link);
  wl_list_remove(&hikari_server.request_set_primary_selection.link);
  wl_list_remove(&hikari_server.output_layout_change.link);
#ifdef HAVE_XWAYLAND
  wl_list_remove(&hikari_server.new_xwayland_surface.link);
#endif

  hikari_indicator_fini(&hikari_server.indicator);
  hikari_mark_assign_mode_fini(&hikari_server.mark_assign_mode);

  wlr_seat_destroy(hikari_server.seat);
  wlr_output_layout_destroy(hikari_server.output_layout);

  wl_display_destroy(hikari_server.display);

  hikari_configuration_fini(hikari_configuration);
  hikari_free(hikari_configuration);
  hikari_marks_fini();
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
    group->sheet = NULL;
  }

  return group;
}

void
hikari_server_lock(void *arg)
{
  hikari_unlocker_start();

  hikari_server.locked = true;

  hikari_server_deactivate_cursor();

  if (hikari_server.workspace->focus_view != NULL) {
    hikari_workspace_focus_view(hikari_server.workspace, NULL);
  }

  wlr_seat_pointer_clear_focus(hikari_server.seat);

  struct hikari_output *output = NULL;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    hikari_output_disable(output);
  }
}

void
hikari_server_reload(void *arg)
{
  hikari_configuration_reload();
}

void
hikari_server_unlock(void)
{
  hikari_server.locked = false;

  hikari_server_activate_cursor();

  struct hikari_output *output = NULL;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    hikari_output_enable(output);
  }

  hikari_server_cursor_focus();
}

#define CYCLE_ACTION(n)                                                        \
  void hikari_server_cycle_##n(void *arg)                                      \
  {                                                                            \
    struct hikari_view *view;                                                  \
                                                                               \
    hikari_server_set_cycling();                                               \
    view = hikari_workspace_##n(hikari_server.workspace);                      \
                                                                               \
    if (view != NULL && view != hikari_server.workspace->focus_view) {         \
      hikari_workspace_focus_view(hikari_server.workspace, view);              \
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
CYCLE_ACTION(next_view)
CYCLE_ACTION(prev_view)
#undef CYCLE_ACTION

void
hikari_server_enter_normal_mode(void *arg)
{
  struct hikari_server *server = &hikari_server;

  assert(server->workspace != NULL);

  server->mode->cancel();
  server->mode = (struct hikari_mode *)&server->normal_mode;

  set_cursor_image(server, "left_ptr");

  hikari_server_refresh_indication();
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

  struct wlr_box *geometry = hikari_view_geometry(focus_view);

  hikari_indicator_update_sheet(&hikari_server.indicator,
      geometry,
      workspace->output,
      focus_view->sheet,
      hikari_configuration->indicator_insert,
      hikari_view_is_iconified(focus_view),
      hikari_view_is_floating(focus_view));

  hikari_server.sheet_assign_mode.sheet = focus_view->sheet;
  hikari_server.mode = (struct hikari_mode *)&hikari_server.sheet_assign_mode;
  hikari_server_refresh_indication();
}

void
hikari_server_enter_move_mode(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  hikari_indicator_update(&hikari_server.indicator,
      focus_view,
      hikari_configuration->indicator_insert);

  hikari_view_raise(focus_view);
  hikari_view_top_left_cursor(focus_view);

  hikari_server.mode = (struct hikari_mode *)&hikari_server.move_mode;
  hikari_server_refresh_indication();
}

void
hikari_server_enter_resize_mode(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  hikari_indicator_update(&hikari_server.indicator,
      focus_view,
      hikari_configuration->indicator_insert);

  hikari_view_raise(focus_view);
  hikari_view_bottom_right_cursor(focus_view);

  hikari_server.mode = (struct hikari_mode *)&hikari_server.resize_mode;
  hikari_server_refresh_indication();
}

void
hikari_server_enter_group_assign_mode(void *arg)
{
  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view == NULL) {
    return;
  }

  struct hikari_group_assign_mode *mode = &hikari_server.group_assign_mode;

  mode->group = focus_view->group;
  hikari_server.mode = (struct hikari_mode *)mode;

  struct wlr_box *geometry = hikari_view_border_geometry(focus_view);
  struct hikari_output *output = hikari_server.workspace->output;

  if (focus_view->group == focus_view->sheet->group) {
    hikari_input_buffer_clear(&mode->input_buffer);
    hikari_indicator_update_group(&hikari_server.indicator,
        geometry,
        output,
        " ",
        hikari_configuration->indicator_insert);
  } else {
    hikari_input_buffer_replace(&mode->input_buffer, focus_view->group->name);

    hikari_indicator_update_group(&hikari_server.indicator,
        geometry,
        output,
        focus_view->group->name,
        hikari_configuration->indicator_insert);
  }
}

void
hikari_server_enter_keyboard_grab_mode(void *arg)
{
  struct hikari_workspace *workspace = hikari_server.workspace;

  assert(workspace != NULL);

  if (workspace->focus_view == NULL) {
    return;
  }

  hikari_server.mode = (struct hikari_mode *)&hikari_server.keyboard_grab_mode;

  hikari_server_refresh_indication();
}

void
hikari_server_enter_tiling_mode(void *arg)
{
  struct hikari_workspace *workspace = hikari_server.workspace;

  assert(workspace != NULL);

  if (workspace->sheet->layout == NULL) {
    return;
  }

  if (workspace->focus_view != NULL) {
    hikari_workspace_focus_view(workspace, NULL);
  }

  hikari_server.mode = (struct hikari_mode *)&hikari_server.tiling_mode;

  hikari_output_damage_whole(workspace->output);
}

static void
enter_mark_select(bool swtch)
{
  struct hikari_server *server = &hikari_server;

  assert(server->workspace != NULL);

  server->mark_select_mode.switch_workspace = swtch;
  server->mode = (struct hikari_mode *)&server->mark_select_mode;

  set_cursor_image(server, "link");

  hikari_server_refresh_indication();
}

void
hikari_server_enter_mark_select_mode(void *arg)
{
  enter_mark_select(false);
}

void
hikari_server_enter_mark_select_switch_mode(void *arg)
{
  enter_mark_select(true);
}

void
hikari_server_enter_layout_select_mode(void *arg)
{
  struct hikari_server *server = &hikari_server;

  assert(server->workspace != NULL);

  server->mode = (struct hikari_mode *)&server->layout_select_mode;

  set_cursor_image(server, "context-menu");

  hikari_server_refresh_indication();
}

void
hikari_server_enter_mark_assign_mode(void *arg)
{
  assert(hikari_server.workspace != NULL);

  struct hikari_workspace *workspace = hikari_server.workspace;

  if (workspace->focus_view == NULL) {
    return;
  }

  hikari_server.mark_assign_mode.pending_mark = NULL;
  hikari_server.mode = (struct hikari_mode *)&hikari_server.mark_assign_mode;

  struct wlr_box *geometry = hikari_view_border_geometry(workspace->focus_view);
  struct hikari_output *output = hikari_server.workspace->output;

  struct hikari_mark *mark = workspace->focus_view->mark;

  if (mark == NULL) {
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        " ",
        hikari_configuration->indicator_insert);
  } else {
    hikari_indicator_update_mark(&hikari_server.indicator,
        geometry,
        output,
        mark->name,
        hikari_configuration->indicator_insert);
  }

  hikari_server_refresh_indication();
}

void
hikari_server_refresh_indication(void)
{
  struct hikari_view *view = NULL;
  struct hikari_workspace *workspace = NULL;

  wl_list_for_each (workspace, &hikari_server.workspaces, server_workspaces) {
    wl_list_for_each (view, &workspace->views, workspace_views) {
      hikari_view_damage_whole(view);
    }
  }

  struct hikari_view *focus_view = hikari_server.workspace->focus_view;

  if (focus_view != NULL) {
    hikari_indicator_damage(&hikari_server.indicator, focus_view);
  }
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
  hikari_sheet_reset_layout(hikari_server.workspace->sheet);
}

void
hikari_server_layout_sheet(void *arg)
{
  char layout_register = (char)arg;
  struct hikari_split *split =
      hikari_configuration_lookup_layout(hikari_configuration, layout_register);

  if (split != NULL) {
    hikari_sheet_apply_split(hikari_server.workspace->sheet, split);
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
    if (!hikari_view_is_hidden(view)) {
      hikari_view_raise(view);
    } else {
      hikari_view_show(view);
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
