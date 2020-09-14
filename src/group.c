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

#define GROUP_VIEW(name, link)                                                 \
  struct hikari_view *hikari_group_##name##_view(struct hikari_group *group)   \
  {                                                                            \
    if (!wl_list_empty(&group->visible_views)) {                               \
      struct wl_list *link = group->visible_views.link;                        \
      struct hikari_view *view =                                               \
          wl_container_of(link, view, visible_group_views);                    \
      return view;                                                             \
    }                                                                          \
                                                                               \
    return NULL;                                                               \
  }

GROUP_VIEW(first, next)
GROUP_VIEW(last, prev)
#undef GROUP_VIEW

void
hikari_group_raise(struct hikari_group *group, struct hikari_view *top)
{
  assert(group != NULL);
  assert(top != NULL);

  hikari_view_raise(top);

  struct hikari_view *view, *view_temp;
  wl_list_for_each_reverse_safe (
      view, view_temp, &group->visible_views, visible_group_views) {
    assert(!hikari_view_is_hidden(view));

    if (view != top) {
      hikari_view_raise(view);
    } else {
      break;
    }
  }

  hikari_view_raise(top);
}

void
hikari_group_lower(struct hikari_group *group, struct hikari_view *top)
{
  assert(group != NULL);
  assert(top != NULL);

  hikari_view_lower(top);

  struct hikari_view *view, *view_temp;
  wl_list_for_each_safe (
      view, view_temp, &group->visible_views, visible_group_views) {
    assert(!hikari_view_is_hidden(view));

    if (view != top) {
      hikari_view_lower(view);
    } else {
      break;
    }
  }
}

void
hikari_group_damage(struct hikari_group *group)
{
  struct hikari_view *view;
  wl_list_for_each (view, &group->visible_views, visible_group_views) {
    hikari_view_damage_border(view);
  }
}

void
hikari_group_show(struct hikari_group *group)
{
  struct hikari_view *view, *view_temp;
  wl_list_for_each_reverse_safe (view, view_temp, &group->views, group_views) {
    if (hikari_view_is_hidden(view)) {
      hikari_view_show(view);
    } else {
      break;
    }
  }
}

void
hikari_group_hide(struct hikari_group *group)
{
  struct hikari_view *view, *view_temp;
  wl_list_for_each_safe (
      view, view_temp, &group->visible_views, visible_group_views) {
    hikari_view_hide(view);
  }
}
