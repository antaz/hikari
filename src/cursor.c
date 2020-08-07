#include <hikari/cursor.h>

#include <assert.h>
#include <errno.h>

#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>

#include <hikari/binding.h>
#include <hikari/binding_config.h>
#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/server.h>

static void
motion_absolute_handler(struct wl_listener *listener, void *data);

static void
frame_handler(struct wl_listener *listener, void *data);

static void
motion_handler(struct wl_listener *listener, void *data);

static void
button_handler(struct wl_listener *listener, void *data);

static void
axis_handler(struct wl_listener *listener, void *data);

static void
request_set_cursor_handler(struct wl_listener *listener, void *data);

static void
surface_destroy_handler(struct wl_listener *listener, void *data);

static unsigned long
get_cursor_size(void)
{
  const char *cursor_size = getenv("XCURSOR_SIZE");

  if (cursor_size != NULL && strlen(cursor_size) > 0) {
    errno = 0;
    char *end;
    unsigned long size = strtoul(cursor_size, &end, 10);
    if (*end == '\0' && errno == 0) {
      return size;
    }
  }

  setenv("XCURSOR_SIZE", "16", 1);

  return 16;
}

static const char *
get_cursor_theme(void)
{
  char *cursor_theme = getenv("XCURSOR_THEME");

  if (cursor_theme != NULL && strlen(cursor_theme) > 0) {
    return cursor_theme;
  }

  setenv("XCURSOR_THEME", "Adwaita", 1);

  return "Adwaita";
}

static void
configure_bindings(struct hikari_cursor *cursor, struct wl_list *bindings)
{
  int nr[256] = { 0 };
  struct hikari_binding_config *binding_config;
  wl_list_for_each (binding_config, bindings, link) {
    nr[binding_config->key.modifiers]++;
  }

  for (int mask = 0; mask < 256; mask++) {
    cursor->bindings[mask].nbindings = nr[mask];
    if (nr[mask] != 0) {
      cursor->bindings[mask].bindings =
          hikari_calloc(nr[mask], sizeof(struct hikari_binding));
    } else {
      cursor->bindings[mask].bindings = NULL;
    }

    nr[mask] = 0;
  }

  wl_list_for_each (binding_config, bindings, link) {
    uint8_t mask = binding_config->key.modifiers;
    struct hikari_binding *binding = &cursor->bindings[mask].bindings[nr[mask]];

    binding->action = &binding_config->action;

    switch (binding_config->key.type) {
      case HIKARI_ACTION_BINDING_KEY_KEYCODE:
        binding->keycode = binding_config->key.value.keycode;
        break;

      case HIKARI_ACTION_BINDING_KEY_KEYSYM:
        assert(false);
        break;
    }

    nr[mask]++;
  }
}

void
hikari_cursor_init(
    struct hikari_cursor *cursor, struct wlr_output_layout *output_layout)
{
  struct wlr_cursor *wlr_cursor = wlr_cursor_create();

  wlr_cursor_attach_output_layout(wlr_cursor, output_layout);

  const char *cursor_theme = get_cursor_theme();
  unsigned long cursor_size = get_cursor_size();

  cursor->cursor_mgr = wlr_xcursor_manager_create(cursor_theme, cursor_size);
  wlr_xcursor_manager_load(cursor->cursor_mgr, 1);

  cursor->wlr_cursor = wlr_cursor;

  wl_list_init(&cursor->surface_destroy.link);
  hikari_binding_group_init(cursor->bindings);
}

void
hikari_cursor_configure_bindings(
    struct hikari_cursor *cursor, struct wl_list *bindings)
{
  hikari_binding_group_fini(cursor->bindings);
  hikari_binding_group_init(cursor->bindings);
  configure_bindings(cursor, bindings);
}

void
hikari_cursor_fini(struct hikari_cursor *cursor)
{
  hikari_binding_group_fini(cursor->bindings);
  hikari_cursor_deactivate(cursor);

  wlr_xcursor_manager_destroy(cursor->cursor_mgr);
}

void
hikari_cursor_activate(struct hikari_cursor *cursor)
{
  struct wlr_cursor *wlr_cursor = cursor->wlr_cursor;

  cursor->motion_absolute.notify = motion_absolute_handler;
  wl_signal_add(&wlr_cursor->events.motion_absolute, &cursor->motion_absolute);

  cursor->frame.notify = frame_handler;
  wl_signal_add(&wlr_cursor->events.frame, &cursor->frame);

  cursor->motion.notify = motion_handler;
  wl_signal_add(&wlr_cursor->events.motion, &cursor->motion);

  cursor->button.notify = button_handler;
  wl_signal_add(&wlr_cursor->events.button, &cursor->button);

  cursor->axis.notify = axis_handler;
  wl_signal_add(&wlr_cursor->events.axis, &cursor->axis);

  cursor->request_set_cursor.notify = request_set_cursor_handler;
  wl_signal_add(&hikari_server.seat->events.request_set_cursor,
      &cursor->request_set_cursor);

  hikari_cursor_reset_image(cursor);
}

void
hikari_cursor_deactivate(struct hikari_cursor *cursor)
{
  wl_list_remove(&cursor->motion_absolute.link);
  wl_list_remove(&cursor->frame.link);
  wl_list_remove(&cursor->motion.link);
  wl_list_remove(&cursor->button.link);
  wl_list_remove(&cursor->axis.link);
  wl_list_remove(&cursor->request_set_cursor.link);

  hikari_cursor_set_image(cursor, NULL);
}

void
hikari_cursor_set_image(struct hikari_cursor *cursor, const char *path)
{
  wl_list_remove(&cursor->surface_destroy.link);
  wl_list_init(&cursor->surface_destroy.link);

  if (path != NULL) {
    wlr_xcursor_manager_set_cursor_image(
        cursor->cursor_mgr, path, cursor->wlr_cursor);
  } else {
    wlr_cursor_set_image(cursor->wlr_cursor, NULL, 0, 0, 0, 0, 0, 0);
  }
}

void
hikari_cursor_center(struct hikari_cursor *cursor,
    struct hikari_output *output,
    struct wlr_box *geometry)
{
  int x = output->geometry.x + geometry->x + geometry->width / 2;
  int y = output->geometry.y + geometry->y + geometry->height / 2;

  hikari_cursor_warp(cursor, x, y);
}

static void
motion_absolute_handler(struct wl_listener *listener, void *data)
{
  struct hikari_cursor *cursor =
      wl_container_of(listener, cursor, motion_absolute);

  assert(!hikari_server_in_lock_mode());

  struct wlr_event_pointer_motion_absolute *event = data;

  wlr_cursor_warp_absolute(
      cursor->wlr_cursor, event->device, event->x, event->y);

  hikari_server.mode->cursor_move(event->time_msec);
}

static void
frame_handler(struct wl_listener *listener, void *data)
{
  assert(!hikari_server_in_lock_mode());

  wlr_seat_pointer_notify_frame(hikari_server.seat);
}

static void
motion_handler(struct wl_listener *listener, void *data)
{
  struct hikari_cursor *cursor = wl_container_of(listener, cursor, motion);

  assert(!hikari_server_in_lock_mode());

  struct wlr_event_pointer_motion *event = data;

  wlr_cursor_move(
      cursor->wlr_cursor, event->device, event->delta_x, event->delta_y);

  hikari_server.mode->cursor_move(event->time_msec);
}

static void
button_handler(struct wl_listener *listener, void *data)
{
  assert(!hikari_server_in_lock_mode());

  struct hikari_cursor *cursor = wl_container_of(listener, cursor, button);
  struct wlr_event_pointer_button *event = data;

  hikari_server.mode->button_handler(cursor, event);
}

static void
axis_handler(struct wl_listener *listener, void *data)
{
  assert(!hikari_server_in_lock_mode());

  struct wlr_event_pointer_axis *event = data;

  wlr_seat_pointer_notify_axis(hikari_server.seat,
      event->time_msec,
      event->orientation,
      event->delta,
      event->delta_discrete,
      event->source);
}

static void
request_set_cursor_handler(struct wl_listener *listener, void *data)
{
  if (!hikari_server_in_normal_mode()) {
    return;
  }

  struct hikari_cursor *cursor =
      wl_container_of(listener, cursor, request_set_cursor);

  struct hikari_server *server = &hikari_server;
  struct wlr_seat_pointer_request_set_cursor_event *event = data;
  struct wlr_seat *seat = server->seat;

  struct wl_client *focused_client = NULL;
  struct wlr_surface *focused_surface = seat->pointer_state.focused_surface;
  if (focused_surface != NULL) {
    focused_client = wl_resource_get_client(focused_surface->resource);
  }

  if (focused_client == NULL || event->seat_client->client != focused_client) {
    return;
  }

  struct wlr_surface *surface = event->surface;

  wl_list_remove(&cursor->surface_destroy.link);
  if (surface != NULL) {
    cursor->surface_destroy.notify = surface_destroy_handler;
    wl_signal_add(&surface->events.destroy, &cursor->surface_destroy);
  } else {
    wl_list_init(&cursor->surface_destroy.link);
  }

  wlr_cursor_set_surface(
      cursor->wlr_cursor, surface, event->hotspot_x, event->hotspot_y);
}

static void
surface_destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_cursor *cursor =
      wl_container_of(listener, cursor, surface_destroy);

  hikari_cursor_reset_image(cursor);
}
