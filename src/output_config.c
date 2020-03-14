#include <hikari/output_config.h>

#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <hikari/memory.h>

void
hikari_output_config_init(struct hikari_output_config *output_config,
    const char *output_name,
    char *background,
    enum hikari_background_fit background_fit,
    bool explicit_position,
    int lx,
    int ly)
{
  size_t output_name_len = strlen(output_name);
  output_config->output_name = hikari_malloc(output_name_len + 1);

  strcpy(output_config->output_name, output_name);
  output_config->background = background;
  output_config->background_fit = background_fit;

  output_config->explicit_position = explicit_position;
  output_config->lx = lx;
  output_config->ly = ly;
}

void
hikari_output_config_fini(struct hikari_output_config *output_config)
{
  assert(output_config != NULL);

  hikari_free(output_config->output_name);
  hikari_free(output_config->background);
}
