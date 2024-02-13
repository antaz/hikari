#include <hikari/switch.h>

#include <hikari/action.h>
#include <hikari/memory.h>
#include <hikari/server.h>
#include <hikari/switch_config.h>

static void
execute_action(void (*action)(void *arg), void *arg)
{
  if (hikari_server_in_lock_mode()) {
    return;
  }

  if (!hikari_server_in_normal_mode()) {
    hikari_server_enter_normal_mode(NULL);
  }

  action(arg);
}

static void
destroy_handler(struct wl_listener *listener, void *data)
{
  struct hikari_switch *swtch = wl_container_of(listener, swtch, destroy);

  hikari_switch_fini(swtch);
}

static void
toggle_handler(struct wl_listener *listener, void *data)
{
  struct hikari_switch *swtch = wl_container_of(listener, swtch, toggle);

  if (swtch->state == WLR_SWITCH_STATE_OFF) {
    struct hikari_event_action *begin = &swtch->action->begin;
    if (begin != NULL) {
      execute_action(begin->action, begin->arg);
    }
    swtch->state = WLR_SWITCH_STATE_ON;
  }

  if (swtch->state == WLR_SWITCH_STATE_ON) {
    struct hikari_event_action *end = &swtch->action->end;
    if (end != NULL) {
      execute_action(end->action, end->arg);
    }
    swtch->state = WLR_SWITCH_STATE_OFF;
  }
}

void
hikari_switch_init(struct hikari_switch *swtch, struct wlr_input_device *device)
{
  struct wlr_switch *wlr_switch = wlr_switch_from_input_device(device);
  swtch->wlr_switch = wlr_switch;
  swtch->state = WLR_SWITCH_STATE_OFF;
  swtch->action = NULL;

  // TODO: How to destroy switch?
  swtch->destroy.notify = destroy_handler;
  // wl_signal_add(&wlr_switch->events.destroy, &swtch->destroy);

  // wl_list_init(&wlr_switch->events.toggle);

  wl_list_insert(&hikari_server.switches, &swtch->server_switches);
}

void
hikari_switch_fini(struct hikari_switch *swtch)
{
  wl_list_remove(&swtch->destroy.link);
  wl_list_remove(&swtch->toggle.link);
  wl_list_remove(&swtch->server_switches);
}

void
hikari_switch_configure(
    struct hikari_switch *swtch, struct hikari_switch_config *switch_config)
{
  swtch->action = &switch_config->action;

  wl_list_remove(&swtch->toggle.link);
  swtch->toggle.notify = toggle_handler;
  wl_signal_add(&swtch->wlr_switch->events.toggle, &swtch->toggle);
}

void
hikari_switch_reset(struct hikari_switch *swtch)
{
  swtch->action = NULL;
}
