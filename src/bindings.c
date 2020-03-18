#include <hikari/bindings.h>

#include <stdlib.h>

#include <hikari/memory.h>

void
hikari_bindings_init(struct hikari_bindings *bindings)
{
  for (int i = 0; i < 256; i++) {
    bindings->keyboard.pressed[i].nbindings = 0;
    bindings->keyboard.pressed[i].bindings = NULL;
    bindings->keyboard.released[i].nbindings = 0;
    bindings->keyboard.released[i].bindings = NULL;

    bindings->mouse.pressed[i].nbindings = 0;
    bindings->mouse.pressed[i].bindings = NULL;
    bindings->mouse.released[i].nbindings = 0;
    bindings->mouse.released[i].bindings = NULL;
  }
}

void
hikari_bindings_fini(struct hikari_bindings *bindings)
{
  for (int i = 0; i < 256; i++) {
    hikari_free(bindings->keyboard.pressed[i].bindings);
    hikari_free(bindings->keyboard.released[i].bindings);

    hikari_free(bindings->mouse.pressed[i].bindings);
    hikari_free(bindings->mouse.released[i].bindings);
  }
}
