#if !defined(HIKARI_ACTION_H)
#define HIKARI_ACTION_H

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

#endif
