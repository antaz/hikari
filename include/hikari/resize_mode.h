#if !defined(HIKARI_RESIZE_MODE_H)
#define HIKARI_RESIZE_MODE_H

#include <hikari/mode.h>

struct hikari_binding;
struct hikari_view;

struct hikari_resize_mode {
  struct hikari_mode mode;
};

void
hikari_resize_mode_init(struct hikari_resize_mode *resize_mode);

void
hikari_resize_mode_enter(struct hikari_view *view);

#endif
