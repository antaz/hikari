#include <hikari/pointer.h>

#include <wlr/backend/libinput.h>

#include <hikari/memory.h>
#include <hikari/pointer_config.h>
#include <hikari/server.h>

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_pointer *pointer = wl_container_of(listener, pointer, destroy);

  hikari_pointer_fini(pointer);
  hikari_free(pointer);
}

void
hikari_pointer_init(
    struct hikari_pointer *pointer, struct wlr_input_device *device)
{
  pointer->device = device;

  wl_list_insert(&hikari_server.pointers, &pointer->server_pointers);

  pointer->destroy.notify = destroy_handler;
  wl_signal_add(&device->events.destroy, &pointer->destroy);
}

void
hikari_pointer_fini(struct hikari_pointer *pointer)
{
  wl_list_remove(&pointer->destroy.link);
  wl_list_remove(&pointer->server_pointers);
}

void
hikari_pointer_configure(struct hikari_pointer *pointer,
    struct hikari_pointer_config *pointer_config)
{
  struct libinput_device *libinput_device =
      wlr_libinput_get_device_handle(pointer->device);

  if (libinput_device != NULL) {
    if (hikari_pointer_config_has_accel(pointer_config)) {
      libinput_device_config_accel_set_speed(
          libinput_device, pointer_config->accel.value);
    }

    if (hikari_pointer_config_has_scroll_button(pointer_config)) {
      libinput_device_config_scroll_set_button(
          libinput_device, pointer_config->scroll_button.value);
    }

    if (hikari_pointer_config_has_disable_while_typing(pointer_config)) {
      if (libinput_device_config_dwt_is_available(libinput_device)) {
        libinput_device_config_dwt_set_enabled(
            libinput_device, pointer_config->disable_while_typing.value);
      }
    }

    if (hikari_pointer_config_has_natural_scrolling(pointer_config)) {
      if (libinput_device_config_scroll_has_natural_scroll(libinput_device)) {
        libinput_device_config_scroll_set_natural_scroll_enabled(
            libinput_device, pointer_config->natural_scrolling.value);
      }
    }

    if (hikari_pointer_config_has_scroll_method(pointer_config)) {
      libinput_device_config_scroll_set_method(
          libinput_device, pointer_config->scroll_method.value);
    }

    if (hikari_pointer_config_has_tap(pointer_config)) {
      if (libinput_device_config_tap_get_finger_count(libinput_device) > 0) {
        libinput_device_config_tap_set_enabled(
            libinput_device, pointer_config->tap.value);
      }
    }

    if (hikari_pointer_config_has_tap_drag(pointer_config)) {
      if (libinput_device_config_tap_get_finger_count(libinput_device) > 0) {
        libinput_device_config_tap_set_drag_enabled(
            libinput_device, pointer_config->tap_drag.value);
      }
    }

    if (hikari_pointer_config_has_tap_drag_lock(pointer_config)) {
      if (libinput_device_config_tap_get_finger_count(libinput_device) > 0) {
        libinput_device_config_tap_set_drag_lock_enabled(
            libinput_device, pointer_config->tap_drag_lock.value);
      }
    }
  }
}
