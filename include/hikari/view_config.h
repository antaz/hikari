#if !defined(HIKARI_VIEW_CONFIG_H)
#define HIKARI_VIEW_CONFIG_H

#include <assert.h>
#include <stdbool.h>
#include <wayland-util.h>

#include <hikari/position_config.h>

struct hikari_mark;
struct hikari_view;

struct hikari_view_config {
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
hikari_view_config_init(struct hikari_view_config *view_config);

void
hikari_view_config_fini(struct hikari_view_config *view_config);

struct hikari_sheet *
hikari_view_config_resolve_sheet(struct hikari_view_config *view_config);

struct hikari_group *
hikari_view_config_resolve_group(
    struct hikari_view_config *view_config, const char *app_id);

void
hikari_view_config_resolve_position(struct hikari_view_config *view_config,
    struct hikari_view *view,
    int *x,
    int *y);

static inline bool
hikari_view_config_resolve_focus(struct hikari_view_config *view_config)
{
  assert(view_config != NULL);
  return view_config->focus;
}

static inline bool
hikari_view_config_resolve_invisible(struct hikari_view_config *view_config)
{
  assert(view_config != NULL);
  return view_config->invisible;
}

static inline bool
hikari_view_config_resolve_floating(struct hikari_view_config *view_config)
{
  assert(view_config != NULL);
  return view_config->floating;
}

static inline bool
hikari_view_config_resolve_public(struct hikari_view_config *view_config)
{
  assert(view_config != NULL);
  return view_config->publicview;
}

static inline struct hikari_mark *
hikari_view_config_resolve_mark(struct hikari_view_config *view_config)
{
  assert(view_config != NULL);
  return view_config->mark;
}

#endif
