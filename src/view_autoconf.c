#include <hikari/view_autoconf.h>

#include <assert.h>
#include <stdlib.h>

#include <hikari/memory.h>

void
hikari_view_autoconf_init(struct hikari_view_autoconf *autoconf)
{
  autoconf->group_name = NULL;
  autoconf->sheet_nr = -1;
  autoconf->mark = NULL;
  autoconf->focus = false;
  autoconf->invisible = false;
  autoconf->floating = false;

  hikari_position_config_init(&autoconf->position);
}

void
hikari_view_autoconf_fini(struct hikari_view_autoconf *autoconf)
{
  assert(autoconf != NULL);

  hikari_free(autoconf->app_id);
  hikari_free(autoconf->group_name);
}
