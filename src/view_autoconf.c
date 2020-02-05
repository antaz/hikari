#include <hikari/view_autoconf.h>

#include <assert.h>
#include <stdlib.h>

#include <hikari/memory.h>

void
hikari_view_autoconf_fini(struct hikari_view_autoconf *autoconf)
{
  assert(autoconf != NULL);

  hikari_free(autoconf->app_id);
  hikari_free(autoconf->group_name);
}
