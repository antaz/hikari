#if !defined(HIKARI_SWITCH_H)
#define HIKARI_SWITCH_H

#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_switch.h>

struct hikari_action;
struct hikari_switch_config;

struct hikari_switch {
  struct wl_list server_switches;

  struct wlr_input_device *device;

  enum wlr_switch_state state;

  struct hikari_action *action;

  struct wl_listener destroy;
  struct wl_listener toggle;
};

void
hikari_switch_init(
    struct hikari_switch *swtch, struct wlr_input_device *device);

void
hikari_switch_fini(struct hikari_switch *swtch);

void
hikari_switch_configure(
    struct hikari_switch *swtch, struct hikari_switch_config *switch_config);

void
hikari_switch_reset(struct hikari_switch *swtch);

#endif
