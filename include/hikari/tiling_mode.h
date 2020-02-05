#if !defined(HIKARI_TILING_MODE_H)
#define HIKARI_TILING_MODE_H

#include <hikari/mode.h>

struct hikari_tiling_mode {
  struct hikari_mode mode;
};

void
hikari_tiling_mode_init(struct hikari_tiling_mode *tiling_mode);

#endif
