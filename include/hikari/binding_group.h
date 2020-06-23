#if !defined(HIKARI_BINDING_GROUP_H)
#define HIKARI_BINDING_GROUP_H

#include <stdint.h>

struct hikari_binding;

#define HIKARI_BINDING_GROUP_MASK 256

struct hikari_binding_group {
  int nbindings;
  struct hikari_binding *bindings;
};

void
hikari_binding_group_init(struct hikari_binding_group *binding_group);

void
hikari_binding_group_fini(struct hikari_binding_group *binding_group);

#endif
