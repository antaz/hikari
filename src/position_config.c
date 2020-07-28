#include <hikari/position_config.h>

#include <string.h>

void
hikari_position_config_init(struct hikari_position_config *position_config)
{
  position_config->type = HIKARI_POSITION_CONFIG_TYPE_AUTO;
}

void
hikari_position_config_set_absolute(
    struct hikari_position_config *position_config, int x, int y)
{
  position_config->type = HIKARI_POSITION_CONFIG_TYPE_ABSOLUTE;

  position_config->config.absolute.x = x;
  position_config->config.absolute.y = y;
}

void
hikari_position_config_set_relative(
    struct hikari_position_config *position_config,
    enum hikari_position_config_relative relative_position)
{
  position_config->type = HIKARI_POSITION_CONFIG_TYPE_RELATIVE;

  position_config->config.relative = relative_position;
}

bool
hikari_position_config_relative_parse(
    struct hikari_position_config *position_config, const char *str)
{
  bool success = false;
  enum hikari_position_config_relative relative_config;

  if (!strcmp("bottom-left", str)) {
    relative_config = HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_LEFT;
    success = true;
  } else if (!strcmp("bottom-middle", str)) {
    relative_config = HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_MIDDLE;
    success = true;
  } else if (!strcmp("bottom-right", str)) {
    relative_config = HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_RIGHT;
    success = true;
  } else if (!strcmp("center", str)) {
    relative_config = HIKARI_POSITION_CONFIG_RELATIVE_CENTER;
    success = true;
  } else if (!strcmp("center-left", str)) {
    relative_config = HIKARI_POSITION_CONFIG_RELATIVE_CENTER_LEFT;
    success = true;
  } else if (!strcmp("center-right", str)) {
    relative_config = HIKARI_POSITION_CONFIG_RELATIVE_CENTER_RIGHT;
    success = true;
  } else if (!strcmp("top-left", str)) {
    relative_config = HIKARI_POSITION_CONFIG_RELATIVE_TOP_LEFT;
    success = true;
  } else if (!strcmp("top-middle", str)) {
    relative_config = HIKARI_POSITION_CONFIG_RELATIVE_TOP_MIDDLE;
    success = true;
  } else if (!strcmp("top-right", str)) {
    relative_config = HIKARI_POSITION_CONFIG_RELATIVE_TOP_RIGHT;
    success = true;
  } else {
    fprintf(stderr, "configuration error: failed to parse \"position\"\n");
  }

  if (success) {
    hikari_position_config_set_relative(position_config, relative_config);
  }

  return success;
}

static bool
parse_position(const ucl_object_t *position_obj, int *x, int *y)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(position_obj);

  int64_t ret_x = 0;
  int64_t ret_y = 0;
  bool parsed_x = false;
  bool parsed_y = false;

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "x")) {
      if (!ucl_object_toint_safe(cur, &ret_x)) {
        fprintf(stderr,
            "configuration error: expected integer for \"x\"-coordinate\n");
        goto done;
      }

      parsed_x = true;
    } else if (!strcmp(key, "y")) {
      if (!ucl_object_toint_safe(cur, &ret_y)) {
        fprintf(stderr,
            "configuration error: expected integer for \"y\"-coordinate\n");
        goto done;
      }

      parsed_y = true;
    } else {
      fprintf(stderr,
          "configuration error: unknown \"position\" key \"%s\"\n",
          key);
      goto done;
    }
  }

  if (!parsed_x) {
    fprintf(stderr,
        "configuration error: missing \"x\"-coordinate in \"position\2\n");
    goto done;
  }

  if (!parsed_y) {
    fprintf(stderr,
        "configuration error: missing \"y\"-coordinate in \"position\"\n");
    goto done;
  }

  success = true;

  *x = ret_x;
  *y = ret_y;

done:
  ucl_object_iterate_free(it);

  return success;
}

bool
hikari_position_config_absolute_parse(
    struct hikari_position_config *position_config,
    const ucl_object_t *position_config_obj)
{
  int x;
  int y;
  bool success = false;

  if (!parse_position(position_config_obj, &x, &y)) {
    fprintf(stderr, "configuration error: failed to parse \"position\"\n");
    goto done;
  }

  hikari_position_config_set_absolute(position_config, x, y);

  success = true;

done:

  return success;
}

bool
hikari_position_config_parse(struct hikari_position_config *position_config,
    const ucl_object_t *position_config_obj)
{
  ucl_type_t type = ucl_object_type(position_config_obj);
  const char *relative;
  bool success = false;

  switch (type) {
    case UCL_OBJECT:
      if (!hikari_position_config_absolute_parse(
              position_config, position_config_obj)) {
        goto done;
      }
      break;

    case UCL_STRING:
      if (!ucl_object_tostring_safe(position_config_obj, &relative) ||
          !hikari_position_config_relative_parse(position_config, relative)) {
        goto done;
      }
      break;

    default:
      fprintf(stderr,
          "configuration error: expected string or object for \"position\"\n");
      goto done;
  }

  success = true;

done:

  return success;
}
