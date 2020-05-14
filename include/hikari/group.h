#if !defined(HIKARIGROUP_H)
#define HIKARIGROUP_H

#include <stdbool.h>

#include <wayland-util.h>

struct hikari_sheet;
struct hikari_view;
struct hikari_workspace;

struct hikari_group {
  char *name;

  struct wl_list views;
  struct wl_list visible_views;

  struct wl_list server_groups;
  struct wl_list visible_server_groups;
};

void
hikari_group_init(struct hikari_group *group, const char *name);

void
hikari_group_fini(struct hikari_group *group);

void
hikari_group_raise(struct hikari_group *group, struct hikari_view *top);

void
hikari_group_lower(struct hikari_group *group, struct hikari_view *top);

void
hikari_group_damage(struct hikari_group *group);

void
hikari_group_show(struct hikari_group *group);

void
hikari_group_hide(struct hikari_group *group);

struct hikari_view *
hikari_group_first_view(struct hikari_group *group);

struct hikari_view *
hikari_group_last_view(struct hikari_group *group);

#endif
