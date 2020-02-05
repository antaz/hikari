#include <hikari/maximized_state.h>

#include <hikari/view.h>

struct hikari_maximized_state *
hikari_maximized_state_full(struct hikari_view *view, int width, int height)
{
  struct hikari_maximized_state *maximized_state;

  if (view->maximized_state == NULL) {
    maximized_state = hikari_maximized_state_alloc();
  } else {
    maximized_state = view->maximized_state;
  }

  maximized_state->maximization = HIKARI_MAXIMIZATION_FULLY_MAXIMIZED;

  maximized_state->geometry.x = 0;
  maximized_state->geometry.y = 0;
  maximized_state->geometry.width = width;
  maximized_state->geometry.height = height;

  return maximized_state;
}
