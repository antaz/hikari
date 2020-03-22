#include <hikari/action.h>

#include <stdlib.h>

void
hikari_action_init(struct hikari_action *action)
{
  action->begin.action = NULL;
  action->begin.arg = NULL;
  action->end.action = NULL;
  action->end.arg = NULL;
}
