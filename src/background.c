#include <hikari/background.h>

#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <hikari/memory.h>

void
hikari_background_init(
    struct hikari_background *background, const char *output_name, char *path)
{
  size_t output_name_len = strlen(output_name);
  background->output_name = hikari_malloc(output_name_len + 1);

  strcpy(background->output_name, output_name);
  background->path = path;
}

void
hikari_background_fini(struct hikari_background *background)
{
  assert(background != NULL);

  hikari_free(background->output_name);
  hikari_free(background->path);
}
