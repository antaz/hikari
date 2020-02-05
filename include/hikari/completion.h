#if !defined(HIKARI_COMPLETION_H)
#define HIKARI_COMPLETION_H

#include <wayland-util.h>

struct hikari_completion_item {
  char data[256];
  struct wl_list completion_items;
};

struct hikari_completion {
  struct hikari_completion_item *current_item;
  struct wl_list items;
};

void
hikari_completion_init(struct hikari_completion *completion, char *data);

void
hikari_completion_fini(struct hikari_completion *completion);

void
hikari_completion_add(struct hikari_completion *completion, char *data);

char *
hikari_completion_cancel(struct hikari_completion *completion);

char *
hikari_completion_next(struct hikari_completion *completion);

char *
hikari_completion_prev(struct hikari_completion *completion);

#endif
