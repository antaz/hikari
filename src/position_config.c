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
    enum hikari_position_config_relative *relative_config, const char *str)
{
  bool success = false;

  if (!strcmp("bottom-left", str)) {
    *relative_config = HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_LEFT;
    success = true;
  } else if (!strcmp("bottom-middle", str)) {
    *relative_config = HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_MIDDLE;
    success = true;
  } else if (!strcmp("bottom-right", str)) {
    *relative_config = HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_RIGHT;
    success = true;
  } else if (!strcmp("center", str)) {
    *relative_config = HIKARI_POSITION_CONFIG_RELATIVE_CENTER;
    success = true;
  } else if (!strcmp("center-left", str)) {
    *relative_config = HIKARI_POSITION_CONFIG_RELATIVE_CENTER_LEFT;
    success = true;
  } else if (!strcmp("center-right", str)) {
    *relative_config = HIKARI_POSITION_CONFIG_RELATIVE_CENTER_RIGHT;
    success = true;
  } else if (!strcmp("top-left", str)) {
    *relative_config = HIKARI_POSITION_CONFIG_RELATIVE_TOP_LEFT;
    success = true;
  } else if (!strcmp("top-middle", str)) {
    *relative_config = HIKARI_POSITION_CONFIG_RELATIVE_TOP_MIDDLE;
    success = true;
  } else if (!strcmp("top-right", str)) {
    *relative_config = HIKARI_POSITION_CONFIG_RELATIVE_TOP_RIGHT;
    success = true;
  }

  return success;
}
