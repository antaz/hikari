#include <hikari/view_config.h>

#include <assert.h>
#include <stdlib.h>

#include <hikari/geometry.h>
#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/server.h>
#include <hikari/view.h>

void
hikari_view_config_init(struct hikari_view_config *view_config)
{
  view_config->group_name = NULL;
  view_config->sheet_nr = -1;
  view_config->mark = NULL;
  view_config->focus = false;
  view_config->invisible = false;
  view_config->floating = false;
  view_config->publicview = false;

  hikari_position_config_init(&view_config->position);
}

void
hikari_view_config_fini(struct hikari_view_config *view_config)
{
  assert(view_config != NULL);

  hikari_free(view_config->app_id);
  hikari_free(view_config->group_name);
}

struct hikari_sheet *
hikari_view_config_resolve_sheet(struct hikari_view_config *view_config)
{
  assert(view_config != NULL);

  struct hikari_sheet *sheet;

  if (view_config->sheet_nr != -1) {
    sheet = hikari_server.workspace->sheets + view_config->sheet_nr;
  } else {
    sheet = hikari_server.workspace->sheet;
  }

  return sheet;
}

struct hikari_group *
hikari_view_config_resolve_group(
    struct hikari_view_config *view_config, const char *app_id)
{
  struct hikari_group *group;

  if (view_config != NULL) {
    if (view_config->group_name != NULL &&
        strlen(view_config->group_name) > 0) {
      group = hikari_server_find_or_create_group(view_config->group_name);
    } else {
      group = hikari_server_find_or_create_group(app_id);
    }
  } else {
    group = hikari_server_find_or_create_group(app_id);
  }

  return group;
}

void
hikari_view_config_resolve_position(struct hikari_view_config *view_config,
    struct hikari_view *view,
    int *x,
    int *y)
{
  assert(view_config != NULL);

  struct hikari_output *output = hikari_server.workspace->output;
  struct wlr_box *geometry = hikari_view_border_geometry(view);

  switch (view_config->position.type) {
    case HIKARI_POSITION_CONFIG_TYPE_ABSOLUTE:
      *x = view_config->position.config.absolute.x;
      *y = view_config->position.config.absolute.y;
      break;

    case HIKARI_POSITION_CONFIG_TYPE_AUTO:
      *x = hikari_server.cursor.wlr_cursor->x - output->geometry.x;
      *y = hikari_server.cursor.wlr_cursor->y - output->geometry.y;
      break;

    case HIKARI_POSITION_CONFIG_TYPE_RELATIVE:
      switch (view_config->position.config.relative) {
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
