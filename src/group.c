#include <hikari/group.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <hikari/memory.h>
#include <hikari/view.h>

void
hikari_group_init(struct hikari_group *group, const char *name)
{
  int length = strlen(name) + 1;

  group->name = hikari_malloc(length);
  strcpy(group->name, name);

  wl_list_init(&group->views);
  wl_list_init(&group->visible_views);

  wl_list_insert(&hikari_server.groups, &group->server_groups);
}

void
hikari_group_fini(struct hikari_group *group)
{
  hikari_free(group->name);
  wl_list_remove(&group->server_groups);
}

struct hikari_view *
hikari_group_first_view(
    struct hikari_group *group, struct hikari_workspace *workspace)
{
  assert(group != NULL);

  struct hikari_view *view = NULL;
  wl_list_for_each (view, &group->visible_views, visible_group_views) {
    if (view->sheet->workspace == workspace) {
      return view;
    }
  }

  return NULL;
}

struct hikari_view *
hikari_group_last_view(
    struct hikari_group *group, struct hikari_workspace *workspace)
{
  assert(group != NULL);

  struct hikari_view *view = NULL;
  wl_list_for_each_reverse (view, &group->visible_views, visible_group_views) {
    if (view->sheet->workspace == workspace) {
      return view;
    }
  }

  return NULL;
}
