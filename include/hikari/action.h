#if !defined(HIKARI_ACTION_H)
#define HIKARI_ACTION_H

#include <stdbool.h>
#include <ucl.h>
#include <wayland-util.h>

struct hikari_event_action {
  void (*action)(void *);
  void *arg;
};

struct hikari_action {
  struct hikari_event_action begin;
  struct hikari_event_action end;
};

void
hikari_action_init(struct hikari_action *action);

bool
hikari_action_parse(struct hikari_action *action,
    struct wl_list *action_configs,
    const ucl_object_t *action_obj);

#endif
