#if !defined(HIKARI_BACKGROUND_H)
#define HIKARI_BACKGROUND_H

#include <wayland-util.h>

struct hikari_background {
  struct wl_list link;

  char *output_name;
  char *path;
};

void
hikari_background_init(
    struct hikari_background *background, const char *output_name, char *path);

void
hikari_background_fini(struct hikari_background *background);

#endif
