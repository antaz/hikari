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
    libinput_device_config_accel_set_speed(
        libinput_device, pointer_config->accel);

    if (pointer_config->scroll_button != 0) {
      libinput_device_config_scroll_set_button(
          libinput_device, pointer_config->scroll_button);
    }

    libinput_device_config_scroll_set_method(
        libinput_device, pointer_config->scroll_method);
  }
}
