#include <hikari/keybinding.h>

#include <assert.h>

#include <hikari/memory.h>

void
hikari_keybinding_fini(struct hikari_keybinding *keybinding)
{
  assert(keybinding != NULL);
  if (keybinding->cleanup != NULL) {
    keybinding->cleanup(keybinding->arg);
  }
}
