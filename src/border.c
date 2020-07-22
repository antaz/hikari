#include <hikari/border.h>

#include <assert.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>

#include <hikari/color.h>
#include <hikari/configuration.h>
#include <hikari/node.h>
#include <hikari/output.h>
#include <hikari/renderer.h>

void
hikari_border_refresh_geometry(
    struct hikari_border *border, struct wlr_box *geometry)
{
  if (border->state == HIKARI_BORDER_NONE) {
    border->geometry = *geometry;
    return;
  }

  int border_width = hikari_configuration->border;

  border->geometry.x = geometry->x - border_width;
  border->geometry.y = geometry->y - border_width;
  border->geometry.width = geometry->width + border_width * 2;
  border->geometry.height = geometry->height + border_width * 2;

  border->top.x = border->geometry.x;
  border->top.y = border->geometry.y;
  border->top.width = border->geometry.width;
  border->top.height = border_width;

  border->bottom.x = border->geometry.x;
  border->bottom.y =
      border->geometry.y + border->geometry.height - border_width;
  border->bottom.width = border->geometry.width;
  border->bottom.height = border_width;

  border->left.x = border->geometry.x;
  border->left.y = border->geometry.y;
  border->left.width = border_width;
  border->left.height = border->geometry.height;

  border->right.x = border->geometry.x + border->geometry.width - border_width;
  border->right.y = border->geometry.y;
  border->right.width = border_width;
  border->right.height = border->geometry.height;
}
