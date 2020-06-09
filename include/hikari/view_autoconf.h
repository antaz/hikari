#if !defined(HIKARI_VIEW_AUTOCONF_H)
#define HIKARI_VIEW_AUTOCONF_H

#include <assert.h>
#include <stdbool.h>
#include <wayland-util.h>

#include <hikari/position_config.h>

struct hikari_mark;
struct hikari_view;

struct hikari_view_autoconf {
  struct wl_list link;

  char *app_id;
  char *group_name;

  int sheet_nr;

  struct hikari_mark *mark;

  struct hikari_position_config position;

  bool focus;
  bool invisible;
  bool floating;
  bool publicview;
};

void
hikari_view_autoconf_init(struct hikari_view_autoconf *view_autoconf);

void
hikari_view_autoconf_fini(struct hikari_view_autoconf *view_autoconf);

struct hikari_sheet *
hikari_view_autoconf_resolve_sheet(struct hikari_view_autoconf *view_autoconf);

struct hikari_group *
hikari_view_autoconf_resolve_group(
    struct hikari_view_autoconf *view_autoconf, const char *app_id);

void
hikari_view_autoconf_resolve_position(
    struct hikari_view_autoconf *view_autoconf,
    struct hikari_view *view,
    int *x,
    int *y);

static inline bool
hikari_view_autoconf_resolve_focus(struct hikari_view_autoconf *view_autoconf)
{
  assert(view_autoconf != NULL);
  return view_autoconf->focus;
}

static inline bool
hikari_view_autoconf_resolve_invisible(
    struct hikari_view_autoconf *view_autoconf)
{
  assert(view_autoconf != NULL);
  return view_autoconf->invisible;
}

static inline bool
hikari_view_autoconf_resolve_floating(
    struct hikari_view_autoconf *view_autoconf)
{
  assert(view_autoconf != NULL);
  return view_autoconf->floating;
}

static inline bool
hikari_view_autoconf_resolve_public(struct hikari_view_autoconf *view_autoconf)
{
  assert(view_autoconf != NULL);
  return view_autoconf->publicview;
}

static inline struct hikari_mark *
hikari_view_autoconf_resolve_mark(struct hikari_view_autoconf *view_autoconf)
{
  assert(view_autoconf != NULL);
  return view_autoconf->mark;
}

#endif
