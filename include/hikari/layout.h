#if !defined(HIKARI_LAYOUT_H)
#define HIKARI_LAYOUT_H

#include <wayland-util.h>

struct hikari_split;
struct hikari_view;

struct hikari_layout {
  struct hikari_split *split;
  struct hikari_sheet *sheet;

  struct wl_list tiles;
};

void
hikari_layout_init(struct hikari_layout *layout,
    struct hikari_split *split,
    struct hikari_sheet *sheet);

void
hikari_layout_fini(struct hikari_layout *layout);

struct hikari_view *
hikari_layout_first_view(struct hikari_layout *layout);

struct hikari_view *
hikari_layout_last_view(struct hikari_layout *layout);

void
hikari_layout_reset(struct hikari_layout *layout);

void
hikari_layout_restack_append(struct hikari_layout *layout);

void
hikari_layout_restack_prepend(struct hikari_layout *layout);

#endif
