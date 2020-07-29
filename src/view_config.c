#include <hikari/view_config.h>

#include <assert.h>
#include <stdlib.h>
#include <ucl.h>

#include <hikari/geometry.h>
#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/server.h>
#include <hikari/view.h>

static void
init_properties(struct hikari_view_properties *properties)
{
  properties->group_name = NULL;
  properties->sheet_nr = -1;
  properties->mark = NULL;
  properties->focus = false;
  properties->invisible = false;
  properties->floating = false;
  properties->publicview = false;

  hikari_position_config_init(&properties->position);
}

void
hikari_view_config_init(struct hikari_view_config *view_config)
{
  view_config->app_id = NULL;

  init_properties(&view_config->properties);

  view_config->child_properties = &view_config->properties;
}

static void
fini_properties(struct hikari_view_properties *properties)
{
  hikari_free(properties->group_name);
}

void
hikari_view_config_fini(struct hikari_view_config *view_config)
{
  assert(view_config != NULL);

  struct hikari_view_properties *properties = &view_config->properties;
  struct hikari_view_properties *child_properties =
      view_config->child_properties;

  if (child_properties != properties) {
    fini_properties(child_properties);
    hikari_free(child_properties);
  }

  fini_properties(properties);
  hikari_free(view_config->app_id);
}

struct hikari_sheet *
hikari_view_properties_resolve_sheet(struct hikari_view_properties *properties)
{
  assert(properties != NULL);

  struct hikari_sheet *sheet;
  if (properties->sheet_nr != -1) {
    sheet = hikari_server.workspace->sheets + properties->sheet_nr;
  } else {
    sheet = hikari_server.workspace->sheet;
  }

  return sheet;
}

struct hikari_group *
hikari_view_properties_resolve_group(
    struct hikari_view_properties *properties, const char *app_id)
{
  struct hikari_group *group;

  if (properties->group_name != NULL && strlen(properties->group_name) > 0) {
    group = hikari_server_find_or_create_group(properties->group_name);
  } else {
    group = hikari_server_find_or_create_group(app_id);
  }

  return group;
}

void
hikari_view_properties_resolve_position(
    struct hikari_view_properties *properties,
    struct hikari_view *view,
    int *x,
    int *y)
{
  assert(properties != NULL);

  struct hikari_output *output = hikari_server.workspace->output;
  struct wlr_box *geometry = hikari_view_border_geometry(view);

  switch (properties->position.type) {
    case HIKARI_POSITION_CONFIG_TYPE_ABSOLUTE:
      *x = properties->position.config.absolute.x;
      *y = properties->position.config.absolute.y;
      break;

    case HIKARI_POSITION_CONFIG_TYPE_AUTO:
      *x = hikari_server.cursor.wlr_cursor->x - output->geometry.x;
      *y = hikari_server.cursor.wlr_cursor->y - output->geometry.y;
      break;

    case HIKARI_POSITION_CONFIG_TYPE_RELATIVE:
      switch (properties->position.config.relative) {
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

static bool
parse_property(struct hikari_view_properties *properties,
    const char *key,
    const ucl_object_t *property_obj)
{
  bool success = false;

  if (!strcmp(key, "group")) {
    const char *group_name;

    if (!ucl_object_tostring_safe(property_obj, &group_name)) {
      fprintf(stderr,
          "configuration error: expected string for \"views\" "
          "\"group\"\n");
      goto done;
    }

    if (strlen(group_name) == 0) {
      fprintf(stderr,
          "configuration error: expected non-empty string for \"views\" "
          "\"group\"\n");
      goto done;
    }

    hikari_free(properties->group_name);
    properties->group_name = strdup(group_name);
  } else if (!strcmp(key, "sheet")) {
    int64_t sheet_nr;
    if (!ucl_object_toint_safe(property_obj, &sheet_nr)) {
      fprintf(stderr,
          "configuration error: expected integer for \"views\" "
          "\"sheet\"\n");
      goto done;
    }

    properties->sheet_nr = sheet_nr;
  } else if (!strcmp(key, "mark")) {
    const char *mark_name;

    if (!ucl_object_tostring_safe(property_obj, &mark_name)) {
      fprintf(stderr,
          "configuration error: expected string for \"views\" \"mark\"");
      goto done;
    }

    if (strlen(mark_name) != 1) {
      fprintf(stderr,
          "configuration error: invalid \"mark\" register \"%s\" for "
          "\"views\"\n",
          mark_name);
      goto done;
    }

    properties->mark = &hikari_marks[mark_name[0] - 'a'];
  } else if (!strcmp(key, "position")) {
    if (!hikari_position_config_parse(&properties->position, property_obj)) {
      goto done;
    }
  } else if (!strcmp(key, "focus")) {
    bool focus;

    if (!ucl_object_toboolean_safe(property_obj, &focus)) {
      fprintf(stderr,
          "configuration error: expected boolean for \"views\" "
          "\"focus\"\n");
      goto done;
    }

    properties->focus = focus;
  } else if (!strcmp(key, "invisible")) {
    bool invisible;

    if (!ucl_object_toboolean_safe(property_obj, &invisible)) {
      fprintf(stderr,
          "configuration error: expected boolean for \"views\" "
          "\"invisible\"\n");
      goto done;
    }

    properties->invisible = invisible;
  } else if (!strcmp(key, "floating")) {
    bool floating;

    if (!ucl_object_toboolean_safe(property_obj, &floating)) {
      fprintf(stderr,
          "configuration error: expected boolean for \"views\" "
          "\"floating\"\n");
      goto done;
    }

    properties->floating = floating;
  } else if (!strcmp(key, "public")) {
    bool publicview;

    if (!ucl_object_toboolean_safe(property_obj, &publicview)) {
      fprintf(stderr,
          "configuration error: expected boolean for \"views\" "
          "\"public\"\n");
      goto done;
    }

    properties->publicview = publicview;
  } else {
    fprintf(stderr, "configuration error: unkown \"views\" key \"%s\"\n", key);
    goto done;
  }

  success = true;

done:

  return success;
}

static bool
parse_properties(struct hikari_view_properties *properties,
    const ucl_object_t *view_config_obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(view_config_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "inherit")) {
      continue;
    }

    if (!parse_property(properties, key, cur)) {
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_inherited_properties(struct hikari_view_properties *properties,
    struct hikari_view_properties *child_properties,
    const ucl_object_t *inherit_obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(inherit_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    ucl_type_t type = ucl_object_type(cur);
    const char *attr;

    switch (type) {
      case UCL_STRING:
        if (!ucl_object_tostring_safe(cur, &attr)) {
          goto done;
        }

        if (!strcmp(attr, "group")) {
          if (properties->group_name != NULL) {
            child_properties->group_name = strdup(properties->group_name);
          } else {
            child_properties->group_name = NULL;
          }
        } else if (!strcmp(attr, "sheet")) {
          child_properties->sheet_nr = properties->sheet_nr;
        } else if (!strcmp(attr, "mark")) {
          child_properties->mark = properties->mark;
        } else if (!strcmp(attr, "position")) {
          memcpy(&child_properties->position,
              &properties->position,
              sizeof(struct hikari_position_config));
        } else if (!strcmp(attr, "focus")) {
          child_properties->focus = properties->focus;
        } else if (!strcmp(attr, "invisible")) {
          child_properties->invisible = properties->invisible;
        } else if (!strcmp(attr, "floating")) {
          child_properties->floating = properties->floating;
        } else if (!strcmp(attr, "publicview")) {
          child_properties->publicview = properties->publicview;
        }
        break;

      case UCL_OBJECT:
        parse_properties(child_properties, cur);
        break;

      default:
        goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

bool
hikari_view_config_parse(
    struct hikari_view_config *view_config, const ucl_object_t *view_config_obj)
{
  struct hikari_view_properties *properties = &view_config->properties;
  bool success = false;

  if (!parse_properties(properties, view_config_obj)) {
    goto done;
  }

  const ucl_object_t *inherit_obj =
      ucl_object_lookup(view_config_obj, "inherit");

  if (inherit_obj != NULL) {
    view_config->child_properties =
        hikari_malloc(sizeof(struct hikari_view_properties));

    struct hikari_view_properties *child_properties =
        view_config->child_properties;
    init_properties(child_properties);

    if (!parse_inherited_properties(
            properties, child_properties, inherit_obj)) {
      goto done;
    }
  }

  success = true;

done:

  return success;
}
