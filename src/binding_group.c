#include <hikari/binding_group.h>

#include <stdlib.h>

#include <hikari/binding.h>
#include <hikari/memory.h>

void
hikari_binding_group_init(struct hikari_binding_group *binding_group)
{
  for (int i = 0; i < HIKARI_BINDING_GROUP_MASK; i++) {
    binding_group[i].nbindings = 0;
    binding_group[i].bindings = NULL;
  }
}

void
hikari_binding_group_fini(struct hikari_binding_group *binding_group)
{
  for (int i = 0; i < HIKARI_BINDING_GROUP_MASK; i++) {
    struct hikari_binding *bindings = binding_group[i].bindings;
    hikari_free(bindings);
  }
}
