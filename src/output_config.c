#include <hikari/output_config.h>

#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <hikari/memory.h>

void
hikari_output_config_init(
    struct hikari_output_config *output_config, const char *output_name)
{
  size_t output_name_len = strlen(output_name);
  output_config->output_name = hikari_malloc(output_name_len + 1);

  strcpy(output_config->output_name, output_name);

  output_config->background.configured = false;
  output_config->background.value = NULL;

  output_config->background_fit.configured = false;
  output_config->background_fit.value = HIKARI_BACKGROUND_STRETCH;

  output_config->position.configured = false;
  hikari_position_config_init(&output_config->position.value);
}

void
hikari_output_config_fini(struct hikari_output_config *output_config)
{
  assert(output_config != NULL);

  hikari_free(output_config->output_name);

  hikari_free(output_config->background.value);
}

void
hikari_output_config_merge(struct hikari_output_config *output_config,
    struct hikari_output_config *default_config)
{
#define MERGE(option)                                                          \
  hikari_output_config_merge_##option(output_config, default_config);

  MERGE(background);
  MERGE(background_fit);
  MERGE(position);
#undef MERGE
}
