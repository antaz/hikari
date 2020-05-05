#if !defined(HIKARI_VIEW_AUTOCONF_H)
#define HIKARI_VIEW_AUTOCONF_H

#include <stdbool.h>
#include <wayland-util.h>

#include <hikari/position_config.h>

struct hikari_mark;

struct hikari_view_autoconf {
  struct wl_list link;

  char *app_id;
  char *group_name;

  int sheet_nr;

  struct hikari_mark *mark;

  struct hikari_position_config position;

  bool focus;
  bool invisible;
};

void
hikari_view_autoconf_init(struct hikari_view_autoconf *autoconf);

void
hikari_view_autoconf_fini(struct hikari_view_autoconf *autoconf);

#endif
