#if !defined(HIKARI_POSITION_CONFIG_H)
#define HIKARI_POSITION_CONFIG_H

#include <stdbool.h>
#include <ucl.h>

struct hikari_position_config_absolute {
  int x;
  int y;
};

enum hikari_position_config_relative {
  HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_LEFT,
  HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_MIDDLE,
  HIKARI_POSITION_CONFIG_RELATIVE_BOTTOM_RIGHT,
  HIKARI_POSITION_CONFIG_RELATIVE_CENTER,
  HIKARI_POSITION_CONFIG_RELATIVE_CENTER_LEFT,
  HIKARI_POSITION_CONFIG_RELATIVE_CENTER_RIGHT,
  HIKARI_POSITION_CONFIG_RELATIVE_TOP_LEFT,
  HIKARI_POSITION_CONFIG_RELATIVE_TOP_MIDDLE,
  HIKARI_POSITION_CONFIG_RELATIVE_TOP_RIGHT
};

enum hikari_position_config_type {
  HIKARI_POSITION_CONFIG_TYPE_ABSOLUTE,
  HIKARI_POSITION_CONFIG_TYPE_AUTO,
  HIKARI_POSITION_CONFIG_TYPE_RELATIVE
};

struct hikari_position_config {
  enum hikari_position_config_type type;

  union {
    struct hikari_position_config_absolute absolute;
    enum hikari_position_config_relative relative;
  } config;
};

void
hikari_position_config_init(struct hikari_position_config *position_config);

void
hikari_position_config_set_absolute(
    struct hikari_position_config *position_config, int x, int y);

void
hikari_position_config_set_relative(
    struct hikari_position_config *position_config,
    enum hikari_position_config_relative relative_config);

bool
hikari_position_config_relative_parse(
    struct hikari_position_config *position_config, const char *str);

bool
hikari_position_config_absolute_parse(
    struct hikari_position_config *position_config,
    const ucl_object_t *position_config_obj);

bool
hikari_position_config_parse(struct hikari_position_config *position_config,
    const ucl_object_t *position_config_obj);

#endif
