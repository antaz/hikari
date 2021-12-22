#if !defined(HIKARI_OPERATION_H)
#define HIKARI_OPERATION_H

#include <wlr/util/box.h>

struct hikari_tile;

enum hikari_operation_type {
  HIKARI_OPERATION_TYPE_RESIZE,
  HIKARI_OPERATION_TYPE_RESET,
  HIKARI_OPERATION_TYPE_UNMAXIMIZE,
  HIKARI_OPERATION_TYPE_FULL_MAXIMIZE,
  HIKARI_OPERATION_TYPE_VERTICAL_MAXIMIZE,
  HIKARI_OPERATION_TYPE_HORIZONTAL_MAXIMIZE,
  HIKARI_OPERATION_TYPE_TILE
};

struct hikari_operation {
  enum hikari_operation_type type;
  bool dirty;
  bool center;
  uint32_t serial;
  struct wlr_box geometry;
  struct hikari_tile *tile;
};

#endif
