#include <hikari/bindings.h>

#include <stdlib.h>

#include <hikari/keybinding.h>
#include <hikari/memory.h>

void
hikari_bindings_init(struct hikari_bindings *bindings)
{
  for (int i = 0; i < 256; i++) {
    bindings->keyboard[i].nbindings = 0;
    bindings->keyboard[i].bindings = NULL;

    bindings->mouse[i].nbindings = 0;
    bindings->mouse[i].bindings = NULL;
  }
}

static void
free_bindings(struct hikari_modifier_bindings *modifier_bindings)
{
  for (int i = 0; i < modifier_bindings->nbindings; i++) {
    hikari_free(modifier_bindings->bindings[i].action);
  }

  hikari_free(modifier_bindings->bindings);
}

void
hikari_bindings_fini(struct hikari_bindings *bindings)
{
  for (int i = 0; i < 256; i++) {
    free_bindings(&bindings->keyboard[i]);
    free_bindings(&bindings->mouse[i]);
  }
}
