#if !defined(HIKARI_DND_MODE_H)
#define HIKARI_DND_MODE_H

#include <hikari/mode.h>

struct hikari_binding;
struct hikari_view;

struct hikari_dnd_mode {
  struct hikari_mode mode;
};

void
hikari_dnd_mode_init(struct hikari_dnd_mode *dnd_mode);

void
hikari_dnd_mode_enter(void);

#endif
