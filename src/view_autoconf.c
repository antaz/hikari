#include <hikari/view_autoconf.h>

#include <assert.h>
#include <stdlib.h>

#include <hikari/geometry.h>
#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/server.h>
#include <hikari/view.h>

void
hikari_view_autoconf_init(struct hikari_view_autoconf *view_autoconf)
{
  view_autoconf->group_name = NULL;
  view_autoconf->sheet_nr = -1;
  view_autoconf->mark = NULL;
  view_autoconf->focus = false;
  view_autoconf->invisible = false;
  view_autoconf->floating = false;
  view_autoconf->publicview = false;

  hikari_position_config_init(&view_autoconf->position);
}

void
hikari_view_autoconf_fini(struct hikari_view_autoconf *view_autoconf)
{
  assert(view_autoconf != NULL);

  hikari_free(view_autoconf->app_id);
  hikari_free(view_autoconf->group_name);
}

struct hikari_sheet *
hikari_view_autoconf_resolve_sheet(struct hikari_view_autoconf *view_autoconf)
{
  assert(view_autoconf != NULL);

  struct hikari_sheet *sheet;

  if (view_autoconf->sheet_nr != -1) {
    sheet = hikari_server.workspace->sheets + view_autoconf->sheet_nr;
  } else {
    sheet = hikari_server.workspace->sheet;
  }

  return sheet;
}

struct hikari_group *
hikari_view_autoconf_resolve_group(
    struct hikari_view_autoconf *view_autoconf, const char *app_id)
{
  struct hikari_group *group;

  if (view_autoconf != NULL) {
    if (view_autoconf->group_name != NULL &&
        strlen(view_autoconf->group_name) > 0) {
      group = hikari_server_find_or_create_group(view_autoconf->group_name);
    } else {
      group = hikari_server_find_or_create_group(app_id);
    }
  } else {
    group = hikari_server_find_or_create_group(app_id);
  }

  return group;
}

void
hikari_view_autoconf_resolve_position(
    struct hikari_view_autoconf *view_autoconf,
    struct hikari_view *view,
    int *x,
    int *y)
{
  assert(view_autoconf != NULL);

  struct hikari_output *output = hikari_server.workspace->output;
  struct wlr_box *geometry = hikari_view_border_geometry(view);

  switch (view_autoconf->position.type) {
    case HIKARI_POSITION_CONFIG_TYPE_ABSOLUTE:
      *x = view_autoconf->position.config.absolute.x;
      *y = view_autoconf->position.config.absolute.y;
      break;

    case HIKARI_POSITION_CONFIG_TYPE_AUTO:
      *x = hikari_server.cursor.wlr_cursor->x - output->geometry.x;
      *y = hikari_server.cursor.wlr_cursor->y - output->geometry.y;
      break;

    case HIKARI_POSITION_CONFIG_TYPE_RELATIVE:
      switch (view_autoconf->position.config.relative) {
        case HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_LEFT:
          hikari_geometry_position_bottom_left(
              geometry, &output->usable_area, x, y);
          break;

        case HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_MIDDLE:
          hikari_geometry_position_bottom_middle(
              geometry, &output->usable_area, x, y);
          break;

        case HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_RIGHT:
          hikari_geometry_position_bottom_right(
              geometry, &output->usable_area, x, y);
          break;

        case HIKARI_POSITION_CONFIG_RELATIVE_CENTER:
          hikari_geometry_position_center(geometry, &output->usable_area, x, y);
          break;

        case HIKARI_POSITION_CONFIG_RELATIVE_CENTER_LEFT:
          hikari_geometry_position_center_left(
              geometry, &output->usable_area, x, y);
          break;

        case HIKARI_POSITION_CONFIG_RELATIVE_CENTER_RIGHT:
          hikari_geometry_position_center_right(
              geometry, &output->usable_area, x, y);
          break;

        case HIKARI_POSITION_CONFIG_RELATIVE_TOP_LEFT:
          hikari_geometry_position_top_left(
              geometry, &output->usable_area, x, y);
          break;

        case HIKARI_POSITION_CONFIG_RELATIVE_TOP_MIDDLE:
          hikari_geometry_position_top_middle(
              geometry, &output->usable_area, x, y);
          break;

        case HIKARI_POSITION_CONFIG_RELATIVE_TOP_RIGHT:
          hikari_geometry_position_top_right(
              geometry, &output->usable_area, x, y);
      }
      break;
  }
}
