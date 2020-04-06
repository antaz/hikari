#include <hikari/decoration.h>

#include <hikari/memory.h>

static void
set_mode(struct hikari_decoration *decoration)
{
  wlr_xdg_toplevel_decoration_v1_set_mode(
      decoration->decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

static void
request_mode_handler(struct wl_listener *listener, void *data)
{
  struct hikari_decoration *decoration =
      wl_container_of(listener, decoration, request_mode);

  set_mode(decoration);
}

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_decoration *decoration =
      wl_container_of(listener, decoration, destroy);

  wl_list_remove(&decoration->request_mode.link);
  wl_list_remove(&decoration->destroy.link);

  hikari_free(decoration);
}

void
hikari_decoration_init(struct hikari_decoration *decoration,
    struct wlr_xdg_toplevel_decoration_v1 *wlr_decoration)
{
  decoration->decoration = wlr_decoration;

  wl_signal_add(
      &wlr_decoration->events.request_mode, &decoration->request_mode);
  decoration->request_mode.notify = request_mode_handler;

  wl_signal_add(&wlr_decoration->events.destroy, &decoration->destroy);
  decoration->destroy.notify = destroy_handler;

  set_mode(decoration);
}
