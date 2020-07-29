#if !defined(HIKARI_VIEW_CONFIG_H)
#define HIKARI_VIEW_CONFIG_H

#include <assert.h>
#include <stdbool.h>
#include <wayland-util.h>

#include <hikari/position_config.h>

struct hikari_mark;
struct hikari_view;

struct hikari_view_properties {
  char *group_name;
  int sheet_nr;
  struct hikari_mark *mark;
  struct hikari_position_config position;
  bool focus;
  bool invisible;
  bool floating;
  bool publicview;
};

struct hikari_view_config {
  struct wl_list link;

  char *app_id;

  struct hikari_view_properties properties;
  struct hikari_view_properties *child_properties;
};

void
hikari_view_config_init(struct hikari_view_config *view_config);

void
hikari_view_config_fini(struct hikari_view_config *view_config);

static inline struct hikari_view_properties *
hikari_view_config_resolve_properties(
    struct hikari_view_config *view_config, bool child)
{
  return child ? view_config->child_properties : &view_config->properties;
}

struct hikari_sheet *
hikari_view_properties_resolve_sheet(struct hikari_view_properties *properties);

struct hikari_group *
hikari_view_properties_resolve_group(
    struct hikari_view_properties *properties, const char *app_id);

void
hikari_view_properties_resolve_position(
    struct hikari_view_properties *properties,
    struct hikari_view *view,
    int *x,
    int *y);

bool
hikari_view_config_parse(struct hikari_view_config *view_config,
    const ucl_object_t *view_config_obj);

#endif
