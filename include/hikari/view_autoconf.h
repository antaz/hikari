#if !defined(HIKARI_VIEW_AUTOCONF_H)
#define HIKARI_VIEW_AUTOCONF_H

#include <stdbool.h>
#include <wayland-util.h>

struct hikari_mark;

struct hikari_view_autoconf {
  struct wl_list link;

  char *app_id;
  char *group_name;

  int sheet_nr;

  struct hikari_mark *mark;

  struct {
    int x;
    int y;
  } position;

  bool focus;
};

void
hikari_view_autoconf_fini(struct hikari_view_autoconf *autoconf);

#endif
